#ifndef BLAZE_DATABASE_H
#define BLAZE_DATABASE_H

#include <blaze/db_result.h>
#include <blaze/traits.h>
#include <boost/asio/awaitable.hpp>
#include <string>
#include <memory>
#include <vector>
#include <type_traits>
#include <functional>
#include <tuple>
#include <any>

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
     * @brief Low-level Transaction Hook (Internal Use)
     */
    virtual boost::asio::awaitable<void> execute_transaction(std::function<boost::asio::awaitable<void>(Database&)> block) = 0;

    /**
     * @brief Start a managed transaction with auto-injection.
     * 
     * @tparam Func A lambda or function accepting dependencies (e.g. Repository<User>, Database&).
     * @param func The transactional logic.
     */
    template<typename Func>
    boost::asio::awaitable<void> transaction(Func&& func) {
        co_await execute_transaction([f = std::forward<Func>(func)](Database& tx_db) -> boost::asio::awaitable<void> {
            using Traits = function_traits<Func>;
            using ArgsTuple = typename Traits::args_tuple;
            
            // Invoke the user's lambda, injecting dependencies constructed from tx_db
            co_await call_tx_deps(f, tx_db, std::make_index_sequence<std::tuple_size_v<ArgsTuple>>{});
        });
    }

private:
    template<typename Func, size_t... Is>
    static boost::asio::awaitable<void> call_tx_deps(Func& func, Database& tx_db, std::index_sequence<Is...>) {
        using Traits = function_traits<Func>;
        using ArgsTuple = typename Traits::args_tuple;

        auto deps = std::make_tuple(
            [&]() -> std::any {
                using ArgType = std::tuple_element_t<Is, ArgsTuple>;
                using PureType = std::remove_cvref_t<ArgType>;

                // 1. Is it a Database&?
                if constexpr (std::is_same_v<PureType, Database>) {
                    // Pass by reference (safe within this scope)
                    return std::shared_ptr<Database>(&tx_db, [](Database*){});
                }
                // 2. Is it constructible from Database&? (e.g. Repository<T>)
                else if constexpr (std::is_constructible_v<PureType, Database&>) {
                    return std::make_shared<PureType>(tx_db);
                }
                // 3. Is it constructible from shared_ptr<Database>? (Legacy Repository)
                else if constexpr (std::is_constructible_v<PureType, std::shared_ptr<Database>>) {
                     return std::make_shared<PureType>(std::shared_ptr<Database>(&tx_db, [](Database*){}));
                }
                else {
                    // Fallback (might fail at compile time if no default ctor)
                    return std::make_shared<PureType>();
                }
            }()...
        );

        co_await func(
            [&]() -> decltype(auto) {
                using ArgType = std::tuple_element_t<Is, ArgsTuple>;
                using PureType = std::remove_cvref_t<ArgType>;
                
                auto val = std::get<Is>(deps);
                
                if constexpr (is_shared_ptr_v<ArgType>) {
                    // User asked for shared_ptr
                    return std::any_cast<ArgType>(val);
                } else {
                    // User asked for T or T&
                    using TargetType = std::shared_ptr<PureType>;
                    return *std::any_cast<TargetType>(val);
                }
            }()... 
        );
    }

public:
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

    // Variadic Helper: query<User>("SELECT...", 1, 2)
    template<typename T, typename... Args,
             typename = std::enable_if_t<(
                sizeof...(Args) > 0 && 
                !(sizeof...(Args) == 1 && (std::is_same_v<std::decay_t<Args>, std::vector<std::string>> && ...))
             )>>
    boost::asio::awaitable<std::vector<T>> query(const std::string& sql, Args&&... args) {
        std::vector<std::string> params = { to_string_param(args)... };
        co_return co_await query<T>(sql, params);
    }
};

} // namespace blaze

#endif
