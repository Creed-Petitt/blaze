#include <catch2/catch_test_macros.hpp>
#include <blaze/db_result.h>
#include <blaze/model.h>
#include <blaze/database.h>
#include <blaze/repository.h>
#include <blaze/router.h>
#include <boost/asio.hpp>
#include <memory>

using namespace blaze;

//MOCK DATABASE CLASSES FOR TESTING
// MockRowImpl inherits from the internal abstract class
class MockRowImpl : public RowImpl {
public:
    std::string_view get_column(size_t index) const override { return "100"; }
    
    std::string_view get_column(std::string_view name) const override {
        if (name == "id") return "42";
        if (name == "name") return "Blaze";
        return "";
    }
    
    bool is_null(size_t index) const override { return false; }
    bool is_null(std::string_view name) const override { return false; }
};

// MockResult inherits from ResultImpl
class MockResult : public ResultImpl {
public:
    size_t size() const override { return 1; }
    
    std::shared_ptr<RowImpl> get_row(size_t index) const override {
        return std::make_shared<MockRowImpl>();
    }
    
    bool is_ok() const override { return true; }
    std::string error_message() const override { return ""; }
};

// Mock Database for capturing params
class SpyDatabase : public Database {
public:
    using Database::query; 
    std::vector<std::string> last_params;
    std::string last_sql;

    std::string placeholder(int index) const override { return "$" + std::to_string(index); }

    Async<DbResult> query(const std::string& sql, const std::vector<std::string>& params = {}) override {
        last_params = params;
        last_sql = sql;
        co_return DbResult(std::make_shared<MockResult>());
    }
};

// --- TEST MODEL ---

struct UserProfile {
    int id;
    std::string name;
};
// Use the macro outside the struct
BLAZE_MODEL(UserProfile, id, name)

TEST_CASE("Database: Decoupled Result and Model Mapping", "[db]") {
    // 1. Create a mocked result set (Value Wrapper around MockResult)
    // We explicitly cast to shared_ptr<ResultImpl> to help the constructor
    std::shared_ptr<ResultImpl> impl = std::make_shared<MockResult>();
    DbResult result(impl);

    SECTION("Value Wrapper Access") {
        // Result[0] returns Row (Value Wrapper)
        Row row = result[0];
        
        // Row["id"] returns Cell
        CHECK(row["id"].as<int>() == 42);
        CHECK(row["name"].as<std::string>() == "Blaze");
    }

    SECTION("Mini ORM / BLAZE_MODEL Mapping") {
        Row row = result[0];
        // Test the hydration
        UserProfile profile = row.as<UserProfile>();
        
        CHECK(profile.id == 42);
        CHECK(profile.name == "Blaze");
    }
}

TEST_CASE("Database: Variadic Query Parameters", "[db]") {
    SpyDatabase db;
    boost::asio::io_context ioc;
    
    // simple flag to check completion
    bool done = false;

    boost::asio::co_spawn(ioc, [&]() -> Async<void> {
        // Test Variadic Expansion: int, const char*, double
        co_await db.query("SELECT ?", 100, "hello", 3.14);
        done = true;
        co_return;
    }, boost::asio::detached);

    // Run until the handlers are executed
    // We limit the run count to prevent infinite blocking
    ioc.run_one(); 
    ioc.poll();

    CHECK(done == true);
    CHECK(db.last_params.size() == 3);
    CHECK(db.last_params[0] == "100");
    CHECK(db.last_params[1] == "hello");
    CHECK(db.last_params[2].substr(0, 4) == "3.14");
}

class SpyRepo : public Repository<UserProfile> {
public:
    using Repository<UserProfile>::Repository;
};

TEST_CASE("Repository: SQL Generation", "[db]") {
    auto db = std::make_shared<SpyDatabase>();
    SpyRepo repo(db);

    boost::asio::io_context ioc;
    
    SECTION("remove(id)") {
        boost::asio::co_spawn(ioc, [&]() -> Async<void> {
            co_await repo.remove(999);
            co_return;
        }, boost::asio::detached);
        ioc.run_one();

        CHECK(db->last_sql.find("DELETE FROM \"UserProfile\"") != std::string::npos);
        CHECK(db->last_params[0] == "999");
    }

    SECTION("count()") {
        boost::asio::co_spawn(ioc, [&]() -> Async<void> {
            co_await repo.count();
            co_return;
        }, boost::asio::detached);
        ioc.run_one();
        
        CHECK(db->last_sql.find("SELECT COUNT(*) FROM \"UserProfile\"") != std::string::npos);
    }

    SECTION("find_where()") {
        boost::asio::co_spawn(ioc, [&]() -> Async<void> {
            co_await repo.find_where("name = $1", "test");
            co_return;
        }, boost::asio::detached);
        ioc.run_one();

        CHECK(db->last_sql.find("SELECT \"id\", \"name\" FROM \"UserProfile\" WHERE name = $1") != std::string::npos);
        CHECK(db->last_params[0] == "test");
    }

    SECTION("fluent query()") {
        boost::asio::co_spawn(ioc, [&]() -> Async<void> {
            co_await repo.query()
                .where("age", ">", 18)
                .order_by("name", "DESC")
                .limit(10)
                .all();
            co_return;
        }, boost::asio::detached);
        ioc.run_one();

        CHECK(db->last_sql.find("WHERE \"age\" > $1") != std::string::npos);
        CHECK(db->last_sql.find("ORDER BY \"name\" DESC") != std::string::npos);
        CHECK(db->last_sql.find("LIMIT 10") != std::string::npos);
        CHECK(db->last_params[0] == "18");
    }
}