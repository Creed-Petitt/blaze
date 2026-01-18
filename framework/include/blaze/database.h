#ifndef BLAZE_DATABASE_H
#define BLAZE_DATABASE_H

#include <blaze/json.h>
#include <boost/asio/awaitable.hpp>
#include <string>

namespace blaze {

class Database {
public:
    virtual ~Database() = default;

    virtual boost::asio::awaitable<Json> query(const std::string& sql) = 0;
};

} // namespace blaze

#endif
