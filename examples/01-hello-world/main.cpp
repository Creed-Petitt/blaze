#include "../../framework/app.h"

int main() {
    App app;

    app.get("/", [](Request& req, Response& res) {
        res.send("Hello, World!");
    });

    app.get("/json", [](Request& req, Response& res) {
        res.json({
            {"message", "Hello, JSON!"},
            {"language", "C++"},
            {"version", "1.0"}
        });
    });

    std::cout << "Server running on http://localhost:3000\n";
    std::cout << "Try: curl http://localhost:3000/\n";
    std::cout << "Try: curl http://localhost:3000/json\n";

    app.listen(3000);
    return 0;
}
