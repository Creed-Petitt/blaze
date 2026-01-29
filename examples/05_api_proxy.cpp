/**
 * Example 05: API Proxy
 * 
 * This example demonstrates how to use the built-in non-blocking HTTP client.
 * Concepts:
 * - blaze::fetch for external API calls
 * - Handling external JSON responses
 * - Propagating headers
 */

#include <blaze/app.h>
#include <blaze/client.h>

using namespace blaze;

// Functions using fetch should always be made named handlers
Async<void> github_handler(Request& req, Response& res) {
    std::string user = req.params["user"];
    std::string url = "https://api.github.com/users/" + user;

    std::map<std::string, std::string> headers;
    headers["User-Agent"] = "Blaze-Proxy-Example";

    auto fetch_res = co_await blaze::fetch(url, "GET", headers);

    if (fetch_res.status == 200) {
        res.json(fetch_res.body);
    } else {
        res.status(fetch_res.status).send("Failed");
    }
    co_return;
}

Async<void> echo_handler(Request& req, Response& res) {
    // Use req.json() to get the blaze::Json wrapper directly
    auto body = req.json();
    auto fetch_res = co_await blaze::fetch("https://httpbin.org/post", "POST", {}, body);
    res.json(fetch_res.body);
    co_return;
}

int main() {
    App app;

    // Route that proxies a request to GitHub
    app.get("/github/:user", github_handler);

    // Route that performs a POST request to an external service
    app.post("/echo", echo_handler);

    std::cout << "Proxy API running on :8080" << std::endl;
    app.listen(8080);

    return 0;
}
