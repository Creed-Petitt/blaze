#include <catch2/catch_test_macros.hpp>
#include <blaze/request.h>
#include <blaze/di.h>

using namespace blaze;

class MyService { public: int val = 42; };

TEST_CASE("Request: Service Resolution", "[di]") {
    Services services;
    services.add<MyService>(std::make_shared<MyService>());

    Request req;
    
    SECTION("Resolve without setting services throws") {
        CHECK_THROWS(req.service<MyService>());
    }

    SECTION("Resolve after setting services works") {
        req._set_services(&services);
        auto svc = req.service<MyService>();
        REQUIRE(svc != nullptr);
        CHECK(svc->val == 42);
    }
}
