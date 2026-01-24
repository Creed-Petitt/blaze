#include <catch2/catch_test_macros.hpp>
#include <blaze/app.h>
#include <blaze/model.h>
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
    app.get("/test", [](Response& res) -> Task {
        res.send("Integration OK");
        co_return;
    });

    // Start server in a background thread
    std::thread server_thread([&]() {
        app.listen(9999); // Use a specific port for testing
    });

    // Give it a moment to ignite
    std::clog << "Waiting for server to start..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    SECTION("Real TCP request should return 200 OK") {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        tcp::socket socket(ioc);

        auto const results = resolver.resolve("127.0.0.1", "9999");
        net::connect(socket, results);

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
    }

    // Stop the server
    app.engine().stop();
    if (server_thread.joinable()) {
        server_thread.join();
    }
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

    // Test Auto-Injection from JSON Body
    app.post("/create_user", [](User user) -> Async<Json> {
        co_return Json({ {"created_id", user.id}, {"created_name", user.name} });
    });

    // Test List of Models (Vector Serialization)
    app.get("/users", []() -> Async<std::vector<User>> {
        co_return std::vector<User>{
            {1, "Alice"},
            {2, "Bob"}
        };
    });

    // Start server (Port 9998 to avoid conflict with 9999)
    std::thread server_thread([&]() {
        app.listen(9998);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    net::io_context ioc;
    tcp::resolver resolver(ioc);
    tcp::socket socket(ioc);
    auto const results = resolver.resolve("127.0.0.1", "9998");
    net::connect(socket, results);

    SECTION("Async<Json> returns valid JSON") {
        std::string req = 
            "GET /json HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Connection: close\r\n\r\n";
        
        net::write(socket, net::buffer(req));

        boost::beast::flat_buffer buffer;
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::http::read(socket, buffer, res);

        CHECK(res.result_int() == 200);
        CHECK(res[boost::beast::http::field::content_type] == "application/json");
        CHECK(res.body() == R"({"val":42})");
    }

    SECTION("Async<User> returns serialized Model") {
        // Reconnect
        socket = tcp::socket(ioc);
        net::connect(socket, results);

        std::string req = 
            "GET /model HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Connection: close\r\n\r\n";
        
        net::write(socket, net::buffer(req));

        boost::beast::flat_buffer buffer;
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::http::read(socket, buffer, res);

        CHECK(res.result_int() == 200);
        CHECK(res.body() == R"({"id":1,"name":"Alice"})");
    }

    SECTION("Auto-Injection of User from JSON Body") {
        // Reconnect
        socket = tcp::socket(ioc);
        net::connect(socket, results);

        std::string body = R"({"id":99, "name":"Bob"})";
        std::string req = 
            "POST /create_user HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Content-Type: application/json\r\n"
            "Connection: close\r\n\r\n" + 
            body;
        
        net::write(socket, net::buffer(req));

        boost::beast::flat_buffer buffer;
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::http::read(socket, buffer, res);

        CHECK(res.result_int() == 200);
        CHECK(res.body() == R"({"created_id":99,"created_name":"Bob"})");
    }

    SECTION("Async<vector<User>> returns JSON Array") {
        // Reconnect
        socket = tcp::socket(ioc);
        net::connect(socket, results);

        std::string req = 
            "GET /users HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Connection: close\r\n\r\n";
        
        net::write(socket, net::buffer(req));

        boost::beast::flat_buffer buffer;
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::http::read(socket, buffer, res);

        CHECK(res.result_int() == 200);
        CHECK(res[boost::beast::http::field::content_type] == "application/json");
        CHECK(res.body() == R"([{"id":1,"name":"Alice"},{"id":2,"name":"Bob"}])");
    }

    app.engine().stop();
    if (server_thread.joinable()) {
        server_thread.join();
    }
}
