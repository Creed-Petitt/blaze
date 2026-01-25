#ifndef BLAZE_DATABASE_H
#define BLAZE_DATABASE_H

#include <blaze/db_result.h>
#include <boost/asio/awaitable.hpp>
#include <string>
#include <memory>
#include <vector>
#include <type_traits>

namespace blaze {

template<typename T>
std::string to_string_param(const T& val) {
    if constexpr (std::is_same_v<T, std::string>) return val;
    else if constexpr (std::is_constructible_v<std::string, T>) return std::string(val);
    else return std::to_string(val);
}

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
    
    /**
     * @brief Returns the parameter placeholder for the specific driver.
     * Postgres: "$1", "$2"
     * MySQL: "?", "?"
     */
    virtual std::string placeholder(int index) const = 0;

    /**
     * @brief Variadic overload for convenient parameter passing.
     * usage: db.query("SELECT * FROM table WHERE id = $1", 100);
     * SFINAE: Disabled if the only argument is already a vector<string> to prevent recursion.
     */
    template<typename... Args, 
             typename = std::enable_if_t<!(sizeof...(Args) == 1 && (std::is_same_v<std::decay_t<Args>, std::vector<std::string>> && ...))>>
    boost::asio::awaitable<DbResult> query(const std::string& sql, Args&&... args) {
        std::vector<std::string> params = { to_string_param(args)... };
        co_return co_await query(sql, params);
    }

    // query<User>("SELECT...", {params})
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

    // query<User>("SELECT...", 1, 2)
    template<typename T, typename... Args,
             typename = std::enable_if_t<!(sizeof...(Args) == 1 && (std::is_same_v<std::decay_t<Args>, std::vector<std::string>> && ...))>>
    boost::asio::awaitable<std::vector<T>> query(const std::string& sql, Args&&... args) {
        std::vector<std::string> params = { to_string_param(args)... };
        co_return co_await query<T>(sql, params);
    }
};

} // namespace blaze

#endif
