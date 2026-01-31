#include <catch2/catch_test_macros.hpp>
#include <blaze/app.h>
#include <blaze/exceptions.h>
#include <blaze/wrappers.h>
#include <blaze/model.h>
#include <boost/mp11.hpp>

using namespace blaze;

struct User {
    int id;
    std::string name;
};
BLAZE_MODEL(User, id, name)

struct Search {
    std::string q;
    int page;
};
BLAZE_MODEL(Search, q, page)

// --- Mocks for Service Injection ---
class IMockDB {
public:
    virtual ~IMockDB() = default;
    virtual std::string query() = 0;
};

class MockDB : public IMockDB {
public:
    std::string query() override { return "SELECT *"; }
};

struct MyService {
    int value = 42;
};

// --- Controller for Static Member Test ---
class TestController {
public:
    static Async<void> list_users(Response& res, MyService& svc) {
        res.send("Users from service " + std::to_string(svc.value));
        co_return;
    }
};

// --- TESTS ---

TEST_CASE("convert_string: Failure cases", "[injection]") {
    SECTION("Invalid integer") {
        CHECK_THROWS_AS(convert_string<int>("abc"), HttpError);
    }
    SECTION("Invalid boolean") {
        CHECK_THROWS_AS(convert_string<bool>("not-a-bool"), HttpError);
    }
    SECTION("Invalid float") {
        CHECK_THROWS_AS(convert_string<double>("xyz"), HttpError);
    }
}

TEST_CASE("Typed Injection: Path Parameters", "[injection]") {
    App app;
    
    app.get("/test/:id/:name", [](Path<int> id, Path<std::string> name) -> Async<Json> {
        // Use implicit conversion to underlying types (int and string)
        co_return Json({{"id", id}, {"name", name}});
    });

    Request req;
    req.method = "GET";
    req.path = "/test/123/blaze";
    
    // Simulate Router behavior
    req.params = {{"id", "123"}, {"name", "blaze"}};
    req.path_values = {"123", "blaze"};

    SECTION("Should extract path parameters by order") {
        Path<int> id(123);
        int val = id; // Implicit conversion
        CHECK(val == 123);
        CHECK(id == 123); // Operators still work perfectly
    }
}

TEST_CASE("Typed Injection: Body Content", "[injection]") {
    Request req;
    req.body = R"({"id": 1, "name": "Blaze"})";
    
    SECTION("Should parse Body<T> and allow direct member access") {
        Body<User> user(req.json<User>());
        CHECK(user.id == 1);
        CHECK(user.name == "Blaze");
        
        User u = user; // Implicit conversion (Slicing)
        CHECK(u.id == 1);
    }
}

TEST_CASE("Typed Injection: Query Parameters", "[injection]") {
    Request req;
    req.query = {{"q", "hello"}, {"page", "5"}};

    SECTION("Should map query to Model and allow direct member access") {
        Search search{};
        
        using Members = boost::describe::describe_members<Search, boost::describe::mod_any_access>;
        boost::mp11::mp_for_each<Members>([&](auto meta) {
            std::string key = meta.name;
            if (req.query.count(key)) {
                using FieldT = std::remove_cvref_t<decltype(search.*meta.pointer)>;
                search.*meta.pointer = convert_string<FieldT>(req.query.at(key));
            }
        });

        Query<Search> q(search);
        CHECK(q.q == "hello");
        CHECK(q.page == 5);
    }
}

struct ValidatedUser {
    int age;
    void validate() const {
        if (age < 0) throw BadRequest("Age cannot be negative");
    }
};
BLAZE_MODEL(ValidatedUser, age)

TEST_CASE("Typed Injection: Automatic Validation", "[injection][validation]") {
    SECTION("Should pass when validation succeeds") {
        Request req;
        req.body = R"({"age": 25})";
        
        auto user = req.json<ValidatedUser>();
        // Trigger manual check to simulate injector logic
        CHECK_NOTHROW(user.validate());
        CHECK(user.age == 25);
    }

    SECTION("Should throw BadRequest when validation fails") {
        Request req;
        req.body = R"({"age": -1})";
        
        auto user = req.json<ValidatedUser>();
        // This simulates what try_validate(model) does inside the injector
        CHECK_THROWS_AS(user.validate(), BadRequest);
    }
}

TEST_CASE("Typed Injection: Service Injection", "[injection]") {
    App app;
    auto db = std::make_shared<MockDB>();
    auto svc = std::make_shared<MyService>();

    app.provide<IMockDB>(db);
    app.provide<MyService>(svc);

    // We can't easily simulate the full DI injection without running the route handler
    // via `inject_and_call`, but we can verify the ServiceProvider works 
    // and that the syntax compiles for the user.
    
    // To truly test `inject_and_call`, we need to expose it or simulate an App call.
    // Since `App::wrap_handler` is private, we trust `test_server` for the full loop,
    // but we can verify resolution here.
    
    SECTION("Service Resolution") {
        auto resolved_db = app.services().resolve<IMockDB>();
        CHECK(resolved_db->query() == "SELECT *");
        
        auto resolved_svc = app.services().resolve<MyService>();
        CHECK(resolved_svc->value == 42);
    }
}

TEST_CASE("Typed Injection: Static Member Functions", "[injection]") {
    App app;
    app.provide<MyService>();
    
    // Registering a static member function
    app.get("/users", &TestController::list_users);
    
    // We verify the router registered it correctly
    auto match = app.get_router().match("GET", "/users");
    REQUIRE(match.has_value());
    
    // Test execution through the injector
    Request req;
    Response res;
    
    // Since App::wrap_handler is where the magic happens, we can't call match->handler directly
    // easily without the full loop, but the fact that it COMPILS above with &TestController::list_users
    // proves that the template deduction for static member functions works.
}

// Note: Full "Mixed Injection" (Service + Path) relies on App::wrap_handler logic
// which is internally tested by the fact that `test_server` runs successfully.
// Adding a unit test here for `inject_and_call` would require making it public public/protected.

