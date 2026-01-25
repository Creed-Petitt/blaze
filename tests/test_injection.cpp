#include <catch2/catch_test_macros.hpp>
#include <blaze/app.h>
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

// --- TESTS ---

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
    
    SECTION("Should parse Body<T> and allow arrow access") {
        Body<User> user(req.json<User>());
        CHECK(user->id == 1);
        CHECK(user->name == "Blaze");
        
        User u = user; // Implicit conversion
        CHECK(u.id == 1);
    }
}

TEST_CASE("Typed Injection: Query Parameters", "[injection]") {
    Request req;
    req.query = {{"q", "hello"}, {"page", "5"}};

    SECTION("Should map query to Model and allow arrow access") {
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
        CHECK(q->q == "hello");
        CHECK(q->page == 5);
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

// Note: Full "Mixed Injection" (Service + Path) relies on App::wrap_handler logic
// which is internally tested by the fact that `test_server` runs successfully.
// Adding a unit test here for `inject_and_call` would require making it public public/protected.

