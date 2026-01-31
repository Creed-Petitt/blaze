#include <catch2/catch_test_macros.hpp>
#include <blaze/app.h>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

using namespace blaze;
namespace net = boost::asio;
namespace websocket = boost::beast::websocket;
using tcp = net::ip::tcp;

TEST_CASE("WebSocket: Connection and Echo", "[websocket]") {
    App app;
    app.config().shutdown_timeout = 0; // Immediate shutdown for tests
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
        try {
            app.listen(8890); // Using unique port
        } catch (...) {}
    });

    struct Guard {
        App& a; std::thread& t;
        ~Guard() { a.stop(); if(t.joinable()) t.join(); }
    } guard{app, server_thread};

    // Wait for start
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    SECTION("Client should connect and exchange messages") {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        websocket::stream<tcp::socket> ws(ioc);

        auto const results = resolver.resolve("127.0.0.1", "8890");
        
        bool success = false;
        for(int i=0; i<20; ++i) {
            try {
                net::connect(ws.next_layer(), results);
                success = true;
                break;
            } catch(...) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        if(!success) FAIL("Could not connect to WS server");

        ws.handshake("127.0.0.1", "/chat");
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        CHECK(connected.load() == true);

        ws.write(net::buffer(std::string("Hello Blaze")));

        boost::beast::flat_buffer buffer;
        ws.read(buffer);

        CHECK(boost::beast::buffers_to_string(buffer.data()) == "Echo: Hello Blaze");

        ws.close(websocket::close_code::normal);
    }
}

TEST_CASE("WebSocket: Automated Broadcasting", "[websocket]") {
    App app;
    app.config().shutdown_timeout = 0; // Immediate shutdown for tests
    
    app.ws("/broadcast", {
        .on_open = [](auto ws) {}
    });

    std::thread server_thread([&]() {
        try {
            app.listen(8891); // Using unique port
        } catch (...) {}
    });

    struct Guard {
        App& a; std::thread& t;
        ~Guard() { a.stop(); if(t.joinable()) t.join(); }
    } guard{app, server_thread};

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    SECTION("Multiple clients should receive broadcasted message") {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        
        // Connect Client 1
        websocket::stream<tcp::socket> ws1(ioc);
        auto const res1 = resolver.resolve("127.0.0.1", "8891");
        
        // Retry Loop 1
        bool s1 = false;
        for(int i=0; i<20; ++i) {
            try { net::connect(ws1.next_layer(), res1); s1=true; break; }
            catch(...) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
        }
        if(!s1) FAIL("Client 1 failed to connect");
        
        ws1.handshake("127.0.0.1", "/broadcast");

        // Connect Client 2
        websocket::stream<tcp::socket> ws2(ioc);
        auto const res2 = resolver.resolve("127.0.0.1", "8891");
        
        // Retry Loop 2
        bool s2 = false;
        for(int i=0; i<20; ++i) {
            try { net::connect(ws2.next_layer(), res2); s2=true; break; }
            catch(...) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
        }
        if(!s2) FAIL("Client 2 failed to connect");
        
        ws2.handshake("127.0.0.1", "/broadcast");

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Use the new Dream API to broadcast
        app.broadcast("/broadcast", "Global Alert");

        // Client 1 should see it
        boost::beast::flat_buffer b1;
        ws1.read(b1);
        CHECK(boost::beast::buffers_to_string(b1.data()) == "\"Global Alert\"");

        // Client 2 should see it
        boost::beast::flat_buffer b2;
        ws2.read(b2);
        CHECK(boost::beast::buffers_to_string(b2.data()) == "\"Global Alert\"");

        ws1.close(websocket::close_code::normal);
        ws2.close(websocket::close_code::normal);
    }
}