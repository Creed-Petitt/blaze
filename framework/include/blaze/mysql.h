#ifndef BLAZE_MYSQL_H
#define BLAZE_MYSQL_H

#include <string>
#include <vector>
#include <memory>
#include <boost/asio/awaitable.hpp>
#include <blaze/json.h>

namespace blaze {

class App;

class MySql {
public:
    // Default pool_size = 10.
    explicit MySql(App& app, std::string url, int pool_size = 10);
    ~MySql();

    void connect();

    // Core Query API
    boost::asio::awaitable<blaze::Json> query(std::string_view sql);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace blaze

#endif