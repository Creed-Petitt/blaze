#include <catch2/catch_test_macros.hpp>
#include <blaze/multipart.h>
#include <blaze/request.h>

using namespace blaze;

TEST_CASE("Multipart: Basic Parsing", "[multipart]") {
    std::string boundary = "boundary123";
    std::string body = 
        "--boundary123\r\n"
        "Content-Disposition: form-data; name=\"field1\"\r\n"
        "\r\n"
        "value1\r\n"
        "--boundary123\r\n"
        "Content-Disposition: form-data; name=\"file1\"; filename=\"test.txt\"\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello World!\r\n"
        "--boundary123--\r\n";

    auto form = multipart::parse(body, boundary);

    SECTION("Should parse regular fields") {
        auto field = form.get_field("field1");
        REQUIRE(field.has_value());
        CHECK(field.value() == "value1");
    }

    SECTION("Should parse files") {
        auto file = form.get_file("file1");
        REQUIRE(file != nullptr);
        CHECK(file->filename == "test.txt");
        CHECK(file->content_type == "text/plain");
        CHECK(file->data == "Hello World!");
    }

    SECTION("Should list all files") {
        auto files = form.files();
        CHECK(files.size() == 1);
    }
}

TEST_CASE("Multipart: Binary Integrity", "[multipart]") {
    std::string boundary = "bin_bound";
    
    // Create a 1KB binary buffer with null bytes and various values
    std::string binary_data;
    binary_data.reserve(1024);
    for (int i = 0; i < 1024; ++i) {
        binary_data.push_back(static_cast<char>(i % 256));
    }

    std::string body = 
        "--bin_bound\r\n"
        "Content-Disposition: form-data; name=\"binary_file\"; filename=\"data.bin\"\r\n"
        "Content-Type: application/octet-stream\r\n"
        "\r\n" + binary_data + "\r\n"
        "--bin_bound--\r\n";

    auto form = multipart::parse(body, boundary);
    auto file = form.get_file("binary_file");

    REQUIRE(file != nullptr);
    CHECK(file->filename == "data.bin");
    CHECK(file->data.size() == binary_data.size());
    
    // Check bit-per-bit equality
    bool match = (file->data == binary_data);
    CHECK(match);
}

TEST_CASE("Multipart: Request Integration", "[multipart][request]") {
    Request req;
    req.body = 
        "--xyz\r\n"
        "Content-Disposition: form-data; name=\"msg\"\r\n"
        "\r\n"
        "Hello\r\n"
        "--xyz--\r\n";
    
    req.headers.set(boost::beast::http::field::content_type, "multipart/form-data; boundary=xyz");

    SECTION("Should parse via Request object") {
        auto val = req.form().get_field("msg");
        REQUIRE(val.has_value());
        CHECK(val.value() == "Hello");
    }
}
