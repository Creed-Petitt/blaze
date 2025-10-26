#ifndef HTTP_SERVER_APP_H
#define HTTP_SERVER_APP_H

#include "router.h"

class App {
private:
    Router router_;

public:
    App() = default;

    void get(const std::string& path, const Handler &handler);
    void post(const std::string& path, Handler handler);
    void put(const std::string& path, Handler handler);
    void del(const std::string& path, Handler handler);

    Router& get_router();
};

#endif