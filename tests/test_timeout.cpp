#include <catch2/catch_test_macros.hpp>
#include <blaze/app.h>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>

using namespace blaze;
namespace net = boost::asio;
using tcp = net::ip::tcp;

TEST_CASE("Server: Timeout Protection (Slowloris)", "[security]") {
    App app;
    // Set aggressive timeout for testing
    app.config().timeout_seconds = 1;

    app.get("/", [](Response& res) -> Task {
        res.send("OK");
        co_return;
    });

    std::thread server_thread([&]() {
        app.listen(9090);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    SECTION("Server should close connection if request is incomplete") {
        net::io_context ioc;
        tcp::socket socket(ioc);
        tcp::resolver resolver(ioc);
        
        net::connect(socket, resolver.resolve("127.0.0.1", "9090"));

        // Send partial request (Slowloris simulation)
        std::string partial = "GET / HTTP/1.1\r\nHost: local";
        net::write(socket, net::buffer(partial));

        // Wait longer than the timeout (1s)
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

        // Try to write more
        boost::system::error_code ec;
        net::write(socket, net::buffer("host\r\n\r\n"), ec);

        // Expectation: 
        // Either Write fails (Broken Pipe) OR Read fails (EOF).
        // Different OS implementations might behave slightly differently,
        // but the connection should be dead.
        
        char buf[1024];
        size_t len = socket.read_some(net::buffer(buf), ec);
        
        // If the server closed it, we should get EOF (ec == net::error::eof) 
        // or Connection Reset.
        bool closed = (ec == net::error::eof || ec == net::error::connection_reset || ec == net::error::broken_pipe);
        
        CHECK(closed == true);
    }

    app.engine().stop();
    if (server_thread.joinable()) server_thread.join();
}
