#ifndef BLAZE_POSTGRES_H
#define BLAZE_POSTGRES_H

#include <blaze/pg_result.h>
#include <blaze/pg_connection.h>
#include <blaze/pg_pool.h>

namespace blaze {
    // Convenience alias: "Postgres" refers to the Pool manager
    using Postgres = PgPool;
}

#endif // BLAZE_POSTGRES_H