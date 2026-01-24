#ifndef BLAZE_DATABASE_H
#define BLAZE_DATABASE_H

#include <blaze/db_result.h>
#include <boost/asio/awaitable.hpp>
#include <string>
#include <memory>

namespace blaze {

/**
 * @brief Abstract interface for database drivers (Postgres, MySQL, etc).
 * 
 * Coding against this interface allows your application to remain database-agnostic.
 */
class Database {
public:
    virtual ~Database() = default;

    /**
     * @brief Executes an asynchronous SQL query with optional parameters.
     * 
     * @param sql The SQL string (use $1, $2 for Postgres or ? for MySQL placeholders).
     * @param params Vector of string parameters to be safely escaped.
     * @return Async<DbResult> yielding a value wrapper.
     */
    virtual boost::asio::awaitable<DbResult> query(const std::string& sql, const std::vector<std::string>& params = {}) = 0;
    
    // Helper: query<User>("SELECT...")
    template<typename T>
    boost::asio::awaitable<std::vector<T>> query(const std::string& sql, const std::vector<std::string>& params = {}) {
        auto res = co_await query(sql, params);
        std::vector<T> vec;
        vec.reserve(res.size());
        for (size_t i = 0; i < res.size(); ++i) {
            vec.push_back(res[i].template as<T>());
        }
        co_return vec;
    }
};

} // namespace blaze

#endif
