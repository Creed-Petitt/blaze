#include "app.h"

void App::get(const std::string& path, const Handler &handler) {
    router_.add_route("GET", path, handler);
}

void App::post(const std::string& path, const Handler handler) {
    router_.add_route("POST", path, handler);
}

void App::put(const std::string& path, const Handler handler) {
    router_.add_route("PUT", path, handler);
}

void App::del(const std::string& path, const Handler handler) {
    router_.add_route("DELETE", path, handler);
}

Router &App::get_router() {
    return router_;
}
