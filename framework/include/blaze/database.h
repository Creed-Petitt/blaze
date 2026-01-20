#ifndef BLAZE_DATABASE_H
#define BLAZE_DATABASE_H

#include <blaze/json.h>
#include <boost/asio/awaitable.hpp>
#include <string>

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
     * @return A Task yielding a blaze::Json wrapper around the result set.
     */
    virtual boost::asio::awaitable<Json> query(const std::string& sql, const std::vector<std::string>& params = {}) = 0;
};

} // namespace blaze

#endif
