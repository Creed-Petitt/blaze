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

class Database {
public:
    std::string query() { return "data"; }
};

TEST_CASE("DI: Service Registration and Resolution", "[di]") {
    ServiceProvider sp;

    SECTION("Singleton registration") {
        auto logger = std::make_shared<ConsoleLogger>();
        sp.provide<ILogger>(logger);

        auto resolved = sp.resolve<ILogger>();
        REQUIRE(resolved != nullptr);
        CHECK(resolved->log() == "logged");
        CHECK(resolved == logger); // Should be the exact same pointer
    }

    SECTION("Resolution of missing service should throw") {
        CHECK_THROWS(sp.resolve<Database>());
    }
}

TEST_CASE("DI: Transient Services", "[di]") {
    ServiceProvider sp;
    int counter = 0;

    sp.provide_transient<Database>([&](ServiceProvider& provider) {
        counter++;
        return std::make_shared<Database>();
    });

    SECTION("Every resolution should create a new instance") {
        auto d1 = sp.resolve<Database>();
        auto d2 = sp.resolve<Database>();
        
        CHECK(counter == 2);
        CHECK(d1 != d2); // Should be different pointers
    }
}