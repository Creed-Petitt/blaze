#include <catch2/catch_test_macros.hpp>
#include <blaze/app.h>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <thread>
#include <atomic>
#include <mutex>

using namespace blaze;
namespace net = boost::asio;
namespace websocket = boost::beast::websocket;
using tcp = net::ip::tcp;

TEST_CASE("WebSocket: Connection and Echo", "[websocket]") {
    App app;
    std::string received_msg;
    std::mutex msg_mutex;
    std::atomic<bool> connected{false};

    app.ws("/chat", {
        .on_open = [&](std::shared_ptr<WebSocket> ws) {
            connected = true;
        },
        .on_message = [&](std::shared_ptr<WebSocket> ws, std::string msg) {
            std::lock_guard<std::mutex> lock(msg_mutex);
            received_msg = msg;
            ws->send("Echo: " + msg);
        }
    });

    std::thread server_thread([&]() {
        app.listen(8888);
    });

    // Wait for start
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    SECTION("Client should connect and exchange messages") {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        websocket::stream<tcp::socket> ws(ioc);

        auto const results = resolver.resolve("127.0.0.1", "8888");
        net::connect(ws.next_layer(), results);

        ws.handshake("127.0.0.1", "/chat");
        
        // Give the server a moment to execute the on_open callback
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        CHECK(connected.load() == true);

        ws.write(net::buffer(std::string("Hello Blaze")));

        boost::beast::flat_buffer buffer;
        ws.read(buffer);

        CHECK(boost::beast::buffers_to_string(buffer.data()) == "Echo: Hello Blaze");
        {
            std::lock_guard<std::mutex> lock(msg_mutex);
            CHECK(received_msg == "Hello Blaze");
        }

        ws.close(websocket::close_code::normal);
    }

    app.engine().stop();
    if (server_thread.joinable()) server_thread.join();
}
