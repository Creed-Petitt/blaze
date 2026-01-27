#include <catch2/catch_test_macros.hpp>
#include <blaze/app.h>

using namespace blaze;

static int register_calls = 0;

class MockController {
public:
    static void register_routes(App& app) {
        register_calls++;
    }
};

class MockController2 {
public:
    static void register_routes(App& app) {
        register_calls++;
    }
};

TEST_CASE("App: Auto-Registration", "[app]") {
    App app;
    register_calls = 0;
    
    app.register_controllers<MockController, MockController2>();
    
    CHECK(register_calls == 2);
}
