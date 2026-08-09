#include <catch2/catch_test_macros.hpp>
#include <blaze/di.h>
#include <memory>

using namespace blaze;

// Simple test services
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual std::string log() = 0;
};

class ConsoleLogger : public ILogger {
public:
    std::string log() override { return "logged"; }
};

class Store {
public:
    std::string query() { return "data"; }
};

TEST_CASE("Services: Registration and Resolution", "[di]") {
    Services services;

    SECTION("Shared service registration") {
        auto logger = std::make_shared<ConsoleLogger>();
        services.add<ILogger>(logger);

        auto resolved = services.get<ILogger>();
        REQUIRE(resolved != nullptr);
        CHECK(resolved->log() == "logged");
        CHECK(resolved == logger);
    }

    SECTION("Resolution of missing service should throw") {
        CHECK_THROWS(services.get<Store>());
    }

    SECTION("Emplace constructs and stores one shared instance") {
        auto created = services.emplace<Store>();
        auto resolved = services.get<Store>();

        CHECK(created == resolved);
        CHECK(resolved->query() == "data");
    }
}
