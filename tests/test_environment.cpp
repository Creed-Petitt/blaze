#include <catch2/catch_test_macros.hpp>
#include <blaze/environment.h>
#include <fstream>
#include <cstdlib>

using namespace blaze;

TEST_CASE("Environment: .env Loading", "[env]") {
    std::string test_file = "test.env";
    
    std::ofstream out(test_file);
    out << "KEY1=VALUE1\n";
    out << "  KEY2 = VALUE2  \n";
    out << "# COMMENT=BLAH\n";
    out << "KEY3=\"QUOTED VALUE\"\n";
    out.close();

    SECTION("Loading and Parsing") {
        REQUIRE(load_env(test_file) == true);
        
        CHECK(std::string(std::getenv("KEY1")) == "VALUE1");
        CHECK(std::string(std::getenv("KEY2")) == "VALUE2");
        CHECK(std::getenv("COMMENT") == nullptr);
        CHECK(std::string(std::getenv("KEY3")) == "QUOTED VALUE");
    }

    SECTION("Non-existent file") {
        CHECK(load_env("missing.env") == false);
    }

    std::remove(test_file.c_str());
}
