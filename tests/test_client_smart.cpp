#include <catch2/catch_test_macros.hpp>
#include <blaze/client.h>
#include <blaze/app.h>
#include <thread>
#include <chrono>

using namespace blaze;

TEST_CASE("Client: Case-Insensitive Headers & Multi-Values", "[client]") {
    // 1. Setup a mini server in a separate thread
    App app;
    int port = 9091; // Use a different port than main tests

    app.get("/headers", [](Response& res) -> Async<void> {
        // Send duplicate headers with mixed casing
        res.add_header("X-Custom-List", "Value1");
        res.add_header("x-custom-list", "Value2");
        
        // Send mixed case single header
        res.header("Content-Type", "application/json");

        res.json({{"status", "ok"}});
        co_return;
    });

    app.get("/timeout", [](Response& res) -> Async<void> {
        // Sleep longer than the client timeout (2s)
        co_await blaze::delay(std::chrono::seconds(5));
        res.send("Too late");
        co_return;
    });

    app.get("/redirect1", [port](Response& res) -> Async<void> {
        res.status(302).header("Location", "http://localhost:" + std::to_string(port) + "/redirect2");
        res.send("Redirecting...");
        co_return;
    });

    app.get("/redirect2", [port](Response& res) -> Async<void> {
        res.status(301).header("Location", "/final");
        res.send("Redirecting again...");
        co_return;
    });

    app.get("/final", [](Response& res) -> Async<void> {
        res.send("Target Reached");
        co_return;
    });

    app.post("/client_upload", [](Request& req) -> Async<Json> {
        const auto& form = req.form();
        auto user = form.get_field("user").value_or("");
        const auto* f = form.get_file("data");
        
        // Debug
        // std::cout << "DEBUG: user=" << user << " file=" << (f ? "yes" : "no") << std::endl;
        // if (f) std::cout << "DEBUG: file data=" << f->data << std::endl;

        if (user == "tester" && f && f->data == "binary_payload") {
            co_return Json({{"status", "ok"}});
        }
        co_return Json({{"status", "fail"}});
    });

    // Run server in background thread
    std::thread server_thread([&app, port]() {
        app.listen(port);
    });

    // Give server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    boost::asio::io_context test_ioc;

    // Test 1: Headers
    boost::asio::co_spawn(test_ioc, [&]() -> boost::asio::awaitable<void> {
        try {
            FetchResponse res;
            int retries = 5;
            while(retries--) {
                try {
                    res = co_await blaze::fetch("http://localhost:" + std::to_string(port) + "/headers");
                    break;
                } catch(...) {
                    if(retries == 0) throw;
                    co_await blaze::delay(std::chrono::milliseconds(100));
                }
            }

            if (res.status != 200) throw std::runtime_error("Bad status");

            // Check Case-Insensitivity
            CHECK(res.get_header("content-type") == "application/json");
            CHECK(res.get_header("CONTENT-TYPE") == "application/json");

            // Check Multiple Values
            auto values = res.get_headers("x-custom-list");
            REQUIRE(values.size() == 2);
            
            bool found1 = false, found2 = false;
            for(const auto& v : values) {
                if (v == "Value1") found1 = true;
                if (v == "Value2") found2 = true;
            }
            CHECK(found1);
            CHECK(found2);

        } catch (const std::exception& e) {
            FAIL(std::string("Headers Test failed: ") + e.what());
        }
    }, boost::asio::detached);

    // Test 2: Timeout
    boost::asio::co_spawn(test_ioc, [&]() -> boost::asio::awaitable<void> {
        bool timed_out = false;
        try {
            // Set timeout to 1 second, server waits 5
            co_await blaze::fetch("http://localhost:" + std::to_string(port) + "/timeout", "GET", {}, {}, 1);
        } catch (const boost::system::system_error& e) {
            if (e.code() == boost::asio::error::timed_out || 
                e.code() == boost::beast::error::timeout) {
                timed_out = true;
            }
        } catch (...) {
            // Any other error
        }
        
        if (!timed_out) {
             FAIL("Client did not timeout as expected");
        }
        CHECK(timed_out);
    }, boost::asio::detached);

    // Test 3: Redirects
    boost::asio::co_spawn(test_ioc, [&]() -> boost::asio::awaitable<void> {
        try {
            // Increase timeout for redirects (it makes 3 hops)
            auto res = co_await blaze::fetch("http://localhost:" + std::to_string(port) + "/redirect1", "GET", {}, {}, 10);
            CHECK(res.status == 200);
            CHECK(res.text() == "Target Reached");
        } catch (const std::exception& e) {
            FAIL(std::string("Redirect Test failed: ") + e.what());
        }
    }, boost::asio::detached);

    // Test 4: Multipart Client
    boost::asio::co_spawn(test_ioc, [&]() -> boost::asio::awaitable<void> {
        try {
            MultipartFormData form;
            form.add_field("user", "tester");
            form.add_file("data", "test.bin", "binary_payload", "application/octet-stream"); // Explicit content type

            // Increase timeout for upload
            auto res = co_await blaze::fetch("http://localhost:" + std::to_string(port) + "/client_upload", form, 10);
            CHECK(res.status == 200);
            
            std::string status = res.json<Json>()["status"].as<std::string>();
            CHECK(status == "ok");

        } catch (const std::exception& e) {
            FAIL(std::string("Multipart Client Test failed: ") + e.what());
        }
    }, boost::asio::detached);

    test_ioc.run();
    app.engine().stop();

    if (server_thread.joinable()) server_thread.join();
}
