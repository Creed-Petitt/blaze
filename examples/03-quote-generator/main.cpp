#include "../../framework/app.h"
#include "../../framework/middleware.h"
#include <vector>
#include <cstdlib>
#include <ctime>

std::vector<std::string> quotes = {
    "The only way to do great work is to love what you do. - Steve Jobs",
    "Code is like humor. When you have to explain it, it's bad. - Cory House",
    "First, solve the problem. Then, write the code. - John Johnson",
    "Experience is the name everyone gives to their mistakes. - Oscar Wilde",
    "In order to be irreplaceable, one must always be different. - Coco Chanel",
    "The best error message is the one that never shows up. - Thomas Fuchs",
    "Simplicity is the soul of efficiency. - Austin Freeman",
    "Make it work, make it right, make it fast. - Kent Beck"
};

int main() {
    App app;
    std::srand(std::time(nullptr));

    // Middleware
    app.use(middleware::cors());
    app.use(middleware::static_files("../public"));  // Automatically serves index.html for /

    // API Routes
    auto api = app.group("/api");

    // Get random quote
    api.get("/quote", [](Request& req, Response& res) {
        int index = std::rand() % quotes.size();
        res.json({
            {"quote", quotes[index]},
            {"id", index},
            {"total", quotes.size()}
        });
    });

    // Get all quotes
    api.get("/quotes", [](Request& req, Response& res) {
        std::vector<nlohmann::json> quote_list;
        for (size_t i = 0; i < quotes.size(); i++) {
            quote_list.push_back({
                {"id", i},
                {"quote", quotes[i]}
            });
        }
        res.json({
            {"quotes", quote_list},
            {"total", quotes.size()}
        });
    });

    // Add new quote
    api.post("/quotes", [](Request& req, Response& res) {
        try {
            auto data = req.json();

            if (!data.contains("quote") || data["quote"].get<std::string>().empty()) {
                res.bad_request("Quote text is required and cannot be empty");
                return;
            }

            std::string new_quote = data["quote"];
            quotes.push_back(new_quote);

            res.status(201).json({
                {"id", quotes.size() - 1},
                {"quote", new_quote},
                {"message", "Quote added successfully"}
            });

        } catch (const std::exception& e) {
            res.bad_request("Invalid JSON in request body");
        }
    });

    std::cout << "\n=== Quote Generator ===\n";
    std::cout << "Server running on http://localhost:3000\n";
    std::cout << "Open your browser to: http://localhost:3000\n\n";
    std::cout << "API endpoints:\n";
    std::cout << "  GET  /api/quote   - Get random quote\n";
    std::cout << "  GET  /api/quotes  - Get all quotes\n";
    std::cout << "  POST /api/quotes  - Add new quote\n\n";

    app.listen(3000);
    return 0;
}
