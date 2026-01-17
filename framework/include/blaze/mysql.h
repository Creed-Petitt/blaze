#ifndef BLAZE_MYSQL_H
#define BLAZE_MYSQL_H

#include <blaze/mysql_result.h>
#include <blaze/mysql_connection.h>
#include <blaze/mysql_pool.h>

namespace blaze {
    // "MySql" refers to the Pool manager
    using MySql = MySqlPool;
}

#endif // BLAZE_MYSQL_H
