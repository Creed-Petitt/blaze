#include <catch2/catch_test_macros.hpp>
#include <blaze/app.h>
#include <blaze/model.h>
#include <blaze/middleware.h>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

using namespace blaze;
namespace net = boost::asio;
using tcp = net::ip::tcp;

struct User {
    int id;
    std::string name;
};
BLAZE_MODEL(User, id, name)

TEST_CASE("Server: End-to-End Request", "[integration]") {
    App app;
    app.get("/test", [](Response& res) -> Async<void> {
        res.send("Integration OK");
        co_return;
    });

    std::thread server_thread([&]() {
        app.listen(9999);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    net::io_context ioc;
    tcp::resolver resolver(ioc);
    tcp::socket socket(ioc);

    auto const results = resolver.resolve("127.0.0.1", "9999");
    net::connect(socket, results);

    SECTION("Real TCP request should return 200 OK") {
        std::string req = 
            "GET /test HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Connection: close\r\n\r\n";
        
        net::write(socket, net::buffer(req));

        boost::beast::flat_buffer buffer;
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::http::read(socket, buffer, res);

        CHECK(res.result() == boost::beast::http::status::ok);
        CHECK(res.body() == "Integration OK");
        CHECK(res[boost::beast::http::field::server] == "Blaze/1.0");
    }

    app.engine().stop();
    if (server_thread.joinable()) server_thread.join();
}

TEST_CASE("Server: Async Return Types", "[app]") {
    App app;
    app.log_to("/dev/null");
    
    app.get("/json", []() -> Async<Json> {
        co_return Json{{"val", 42}};
    });

    app.get("/model", []() -> Async<User> {
        co_return User{1, "Alice"};
    });

    app.post("/create_user", [](User user) -> Async<Json> {
        co_return Json({ {"created_id", user.id}, {"created_name", user.name} });
    });

    app.get("/users", []() -> Async<std::vector<User>> {
        co_return std::vector<User>{ {1, "Alice"}, {2, "Bob"} };
    });

    std::thread server_thread([&]() {
        app.listen(9998);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    net::io_context ioc;
    tcp::resolver resolver(ioc);
    auto const results = resolver.resolve("127.0.0.1", "9998");

    SECTION("Async<Json>") {
        tcp::socket socket(ioc);
        net::connect(socket, results);
        net::write(socket, net::buffer("GET /json HTTP/1.1\r\nHost: localhost\r\n\r\n"));
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::flat_buffer buffer;
        boost::beast::http::read(socket, buffer, res);
        CHECK(res.body() == R"({"val":42})");
    }

    SECTION("Async<User>") {
        tcp::socket socket(ioc);
        net::connect(socket, results);
        net::write(socket, net::buffer("GET /model HTTP/1.1\r\nHost: localhost\r\n\r\n"));
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::flat_buffer buffer;
        boost::beast::http::read(socket, buffer, res);
        CHECK(res.body() == R"({"id":1,"name":"Alice"})");
    }

    SECTION("Auto-Injection") {
        tcp::socket socket(ioc);
        net::connect(socket, results);
        std::string body = R"({"id":99, "name":"Bob"})";
        std::string req = "POST /create_user HTTP/1.1\r\nHost: localhost\r\nContent-Length: " + std::to_string(body.size()) + "\r\nContent-Type: application/json\r\n\r\n" + body;
        net::write(socket, net::buffer(req));
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::flat_buffer buffer;
        boost::beast::http::read(socket, buffer, res);
        CHECK(res.body() == R"({"created_id":99,"created_name":"Bob"})");
    }

    SECTION("Async<vector>") {
        tcp::socket socket(ioc);
        net::connect(socket, results);
        net::write(socket, net::buffer("GET /users HTTP/1.1\r\nHost: localhost\r\n\r\n"));
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::flat_buffer buffer;
        boost::beast::http::read(socket, buffer, res);
        CHECK(res.body() == R"([{"id":1,"name":"Alice"},{"id":2,"name":"Bob"}])");
    }

    app.engine().stop();
    if (server_thread.joinable()) server_thread.join();
}

TEST_CASE("Server: Body Size Limits", "[integration]") {
    App app;
    app.log_to("/dev/null");
    app.max_body_size(100); // 100 bytes limit

    app.post("/small", [](Response& res) -> Async<void> {
        res.send("Received");
        co_return;
    });

    std::thread server_thread([&]() {
        app.listen(9996);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    net::io_context ioc;
    tcp::resolver resolver(ioc);
    auto const results = resolver.resolve("127.0.0.1", "9996");

    SECTION("Small body should be accepted") {
        tcp::socket socket(ioc);
        net::connect(socket, results);
        std::string body = "small";
        std::string req = "POST /small HTTP/1.1\r\nHost: localhost\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
        net::write(socket, net::buffer(req));
        
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::flat_buffer buffer;
        boost::beast::http::read(socket, buffer, res);
        CHECK(res.result_int() == 200);
    }

    SECTION("Large body should be rejected") {
        tcp::socket socket(ioc);
        net::connect(socket, results);
        std::string body(200, 'x'); // 200 bytes > 100 bytes limit
        std::string req = "POST /small HTTP/1.1\r\nHost: localhost\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
        
        boost::beast::error_code ec;
        net::write(socket, net::buffer(req), ec);
        
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::flat_buffer buffer;
        boost::beast::http::read(socket, buffer, res, ec);
        
        // Boost.Beast returns 413 Payload Too Large automatically when limit is exceeded
        CHECK(res.result() == boost::beast::http::status::payload_too_large);
    }

    app.engine().stop();
    if (server_thread.joinable()) server_thread.join();
}

TEST_CASE("Server: Auth & Cookies", "[integration]") {
    App app;
    app.log_to("/dev/null");

    app.use(middleware::bearer_auth([](std::string_view token) {
        return token == "valid-token";
    }));

    app.get("/protected", [](Response& res) -> Async<void> {
        res.send("Secret");
        co_return;
    });

    app.get("/cookie", [](Response& res) -> Async<void> {
        res.set_cookie("session", "xyz", 3600, true, true);
        res.send("ok");
        co_return;
    });

    std::thread server_thread([&]() {
        app.listen(9997);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    net::io_context ioc;
    tcp::resolver resolver(ioc);
    auto const results = resolver.resolve("127.0.0.1", "9997");

    SECTION("Auth: Invalid Token") {
        tcp::socket socket(ioc);
        net::connect(socket, results);
        net::write(socket, net::buffer("GET /protected HTTP/1.1\r\nHost: localhost\r\nAuthorization: Bearer bad\r\n\r\n"));
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::flat_buffer buffer;
        boost::beast::http::read(socket, buffer, res);
        CHECK(res.result_int() == 403);
    }

    SECTION("Auth: Valid Token") {
        tcp::socket socket(ioc);
        net::connect(socket, results);
        net::write(socket, net::buffer("GET /protected HTTP/1.1\r\nHost: localhost\r\nAuthorization: Bearer valid-token\r\n\r\n"));
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::flat_buffer buffer;
        boost::beast::http::read(socket, buffer, res);
        CHECK(res.result_int() == 200);
        CHECK(res.body() == "Secret");
    }

    SECTION("Cookies") {
        tcp::socket socket(ioc);
        net::connect(socket, results);
        net::write(socket, net::buffer("GET /cookie HTTP/1.1\r\nHost: localhost\r\nAuthorization: Bearer valid-token\r\n\r\n"));
        
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::flat_buffer buffer;
        boost::beast::http::read(socket, buffer, res);
        
        CHECK(res.result_int() == 200);
        bool found = false;
        for(auto const& field : res) {
            if (field.name_string() == "Set-Cookie") {
                std::string val(field.value());
                if (val.find("session=xyz") != std::string::npos) found = true;
            }
        }
        CHECK(found);
    }

    app.engine().stop();
    if (server_thread.joinable()) server_thread.join();
}