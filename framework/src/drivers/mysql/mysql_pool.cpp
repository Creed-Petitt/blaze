#define BOOST_MYSQL_SEPARATE_COMPILATION
#define BOOST_CHARCONV_NO_QUADMATH
#include <blaze/mysql.h>
#include <blaze/app.h>
#include <boost/mysql/src.hpp>
#include <boost/mysql.hpp>
#include <boost/charconv/from_chars.hpp>
#include <boost/charconv/to_chars.hpp>
#include <libs/charconv/src/from_chars.cpp>
#include <libs/charconv/src/to_chars.cpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>
#include <regex>
#include <queue>
#include <mutex>

namespace blaze {

struct MySql::Impl {
    boost::asio::io_context& ioc;
    std::string url;
    int pool_size;
    std::vector<std::unique_ptr<boost::mysql::any_connection>> pool;
    std::queue<boost::mysql::any_connection*> available;
    std::mutex mtx;

    Impl(boost::asio::io_context& ctx, std::string u, int sz)
        : ioc(ctx), url(std::move(u)), pool_size(sz) {}

    // Helper to parse URL
    boost::mysql::connect_params parse_url() {
        std::regex url_regex("mysql://([^:]+):([^@]+)@([^:/]+)(?::([0-9]+))?/(.+)");
        std::smatch matches;
        boost::mysql::connect_params params;
        if (std::regex_search(url, matches, url_regex)) {
            params.username = matches[1].str();
            params.password = matches[2].str();
            std::string host = matches[3].str();
            int port = matches[4].matched ? std::stoi(matches[4].str()) : 3306;
            params.database = matches[5].str();
            params.server_address.emplace_host_and_port(host, (unsigned short)port);
        }
        return params;
    }
};

MySql::MySql(App& app, std::string url, int pool_size)
    : impl_(std::make_unique<Impl>(app.engine(), std::move(url), pool_size)) {}

MySql::~MySql() = default;

void MySql::connect() {
    auto params = impl_->parse_url();
    for (int i = 0; i < impl_->pool_size; ++i) {
        auto conn = std::make_unique<boost::mysql::any_connection>(boost::asio::make_strand(impl_->ioc));
        boost::asio::co_spawn(conn->get_executor(), [this, c = conn.get(), params]() -> boost::asio::awaitable<void> {
            try {
                co_await c->async_connect(params, boost::asio::use_awaitable);
                std::lock_guard<std::mutex> lock(impl_->mtx);
                impl_->available.push(c);
            } catch (...) {}
        }, boost::asio::detached);
        impl_->pool.push_back(std::move(conn));
    }
}

boost::asio::awaitable<boost::json::value> MySql::query(std::string_view sql) {
    boost::mysql::any_connection* conn = nullptr;
    while (true) {
        {
            std::lock_guard<std::mutex> lock(impl_->mtx);
            if (!impl_->available.empty()) {
                conn = impl_->available.front();
                impl_->available.pop();
                break;
            }
        }
        boost::asio::steady_timer timer(impl_->ioc, std::chrono::milliseconds(1));
        co_await timer.async_wait(boost::asio::use_awaitable);
    }

    try {
        boost::mysql::results res;
        co_await conn->async_execute(sql, res, boost::asio::use_awaitable);
        
        boost::json::array rows;
        for (auto row : res.rows()) {
            boost::json::object json_row;
            for (std::size_t i = 0; i < row.size(); ++i) {
                std::stringstream ss;
                ss << row[i];
                json_row[res.meta()[i].column_name()] = ss.str(); 
            }
            rows.push_back(std::move(json_row));
        }

        std::lock_guard<std::mutex> lock(impl_->mtx);
        impl_->available.push(conn);
        co_return rows;
    } catch (...) {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        impl_->available.push(conn);
        co_return boost::json::array();
    }
}

} // namespace blaze