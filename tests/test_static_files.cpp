#include <catch2/catch_test_macros.hpp>
#include <blaze/app.h>
#include <blaze/middleware.h>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <fstream>
#include <filesystem>

using namespace blaze;
namespace net = boost::asio;
using tcp = net::ip::tcp;

TEST_CASE("Middleware: Static Files Zero-Copy", "[middleware][static]") {
    App app;
    app.log_to("/dev/null");

    // Create a temporary test file
    std::string test_dir = "./test_static";
    std::filesystem::create_directories(test_dir);
    std::string test_file = test_dir + "/hello.txt";
    std::string content = "Zero-copy streaming test content.";
    
    {
        std::ofstream ofs(test_file, std::ios::binary);
        ofs << content;
    }

    app.use(middleware::static_files(test_dir));

    std::thread server_thread([&]() {
        app.listen(9991);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    net::io_context ioc;
    tcp::resolver resolver(ioc);
    auto const results = resolver.resolve("127.0.0.1", "9991");

    SECTION("Serve file correctly via streaming") {
        tcp::socket socket(ioc);
        net::connect(socket, results);

        net::write(socket, net::buffer("GET /hello.txt HTTP/1.1\r\nHost: localhost\r\n\r\n"));
        
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::flat_buffer buffer;
        boost::beast::http::read(socket, buffer, res);

        CHECK(res.result() == boost::beast::http::status::ok);
        CHECK(res.body() == content);
        CHECK(res[boost::beast::http::field::content_type] == "text/plain");
    }

    SECTION("Serve index.html automatically") {
        std::string index_file = test_dir + "/index.html";
        std::string index_content = "<h1>Index Page</h1>";
        {
            std::ofstream ofs(index_file, std::ios::binary);
            ofs << index_content;
        }

        tcp::socket socket(ioc);
        net::connect(socket, results);

        net::write(socket, net::buffer("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"));
        
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::flat_buffer buffer;
        boost::beast::http::read(socket, buffer, res);

        CHECK(res.result() == boost::beast::http::status::ok);
        CHECK(res.body() == index_content);
        CHECK(res[boost::beast::http::field::content_type] == "text/html");
    }

    app.engine().stop();
    if (server_thread.joinable()) server_thread.join();
    
    // Cleanup
    std::filesystem::remove_all(test_dir);
}
