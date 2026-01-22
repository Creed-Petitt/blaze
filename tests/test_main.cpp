#include <catch2/catch_test_macros.hpp>
#include <blaze/db_result.h>
#include <blaze/model.h>
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