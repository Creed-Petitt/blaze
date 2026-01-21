#include <catch2/catch_test_macros.hpp>
#include <blaze/app.h>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <thread>
#include <chrono>

using namespace blaze;
namespace net = boost::asio;
using tcp = net::ip::tcp;

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
