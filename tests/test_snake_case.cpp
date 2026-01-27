#include <catch2/catch_test_macros.hpp>
#include <blaze/repository.h>

using namespace blaze;

struct User {
    // To allow Repository to instantiate, it might need this? 
    // Actually Repository<T> doesn't instantiate T inside infer_table_name.
};
struct UserOrder {};
struct SuperSpecialTable {};
struct HTTPRequest {}; // Added definition

namespace deeply {
    namespace nested {
        struct NamespacedUser {};
    }
}

// To access protected `infer_table_name`, we subclass
template<typename T>
class TestRepo : public Repository<T> {
public:
    // Pass nullptr for DB since we only test table name inference
    TestRepo() : Repository<T>(nullptr) {}
    
    // Expose the protected method
    std::string get_name() { return this->infer_table_name(); }
};

TEST_CASE("Repository: Snake Case Inference", "[model]") {
    SECTION("Simple Class") {
        TestRepo<User> repo;
        CHECK(repo.get_name() == "users");
    }

    SECTION("CamelCase Class") {
        TestRepo<UserOrder> repo;
        CHECK(repo.get_name() == "user_orders");
    }

    SECTION("Acronym Handling") {
        TestRepo<HTTPRequest> repo;
        // Was "h_t_t_p_request", now "http_requests" (pluralized)
        CHECK(repo.get_name() == "http_requests");
    }

    SECTION("Namespace Stripping") {
        TestRepo<deeply::nested::NamespacedUser> repo;
        // Should ignore "deeply::nested::" and just see "NamespacedUser" -> "namespaced_users"
        CHECK(repo.get_name() == "namespaced_users");
    }
}
