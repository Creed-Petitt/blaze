#include <catch2/catch_test_macros.hpp>
#include <blaze/request.h>
#include <blaze/di.h>

using namespace blaze;

class MyService { public: int val = 42; };

TEST_CASE("Request: Service Resolution", "[di]") {
    ServiceProvider sp;
    sp.provide<MyService>(std::make_shared<MyService>());

    Request req;
    
    SECTION("Resolve without setting services throws") {
        CHECK_THROWS(req.resolve<MyService>());
    }

    SECTION("Resolve after setting services works") {
        req._set_services(&sp);
        auto svc = req.resolve<MyService>();
        REQUIRE(svc != nullptr);
        CHECK(svc->val == 42);
    }
}
