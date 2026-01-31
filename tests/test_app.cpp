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

TEST_CASE("App: Configuration & Fluent API", "[app]") {
    App app;
    
    app.server_name("TestServer/2.0")
       .max_body_size(1024)
       .timeout(45)
       .num_threads(16)
       .log_level(LogLevel::DEBUG);

    auto& config = app.get_config();
    CHECK(config.server_name == "TestServer/2.0");
    CHECK(config.max_body_size == 1024);
    CHECK(config.timeout_seconds == 45);
    CHECK(config.num_threads == 16);
    CHECK(Logger::instance().get_level() == LogLevel::DEBUG);
}

TEST_CASE("App: Documentation Toggle", "[app]") {
    SECTION("Docs Enabled (Default)") {
        App app;
        // In our current implementation, docs are registered during listen()
        // but we can check the internal state if we exposed it, or just 
        // rely on the fact that it's enabled by default.
        CHECK(app.get_config().enable_docs == true);
    }

    SECTION("Docs Disabled") {
        App app;
        app.enable_docs(false);
        CHECK(app.get_config().enable_docs == false);
    }
}
