#ifndef HTTP_SERVER_REQUEST_H
#define HTTP_SERVER_REQUEST_H

#include <string>
#include <unordered_map>
#include "../json.hpp"

using json = nlohmann::json;

class Request {
public:
    explicit Request(int client_fd);

    std::string method;
    std::string path;
    std::string http_version;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> params;
    std::unordered_map<std::string, std::string> query;
    std::string body;

    nlohmann::json json() const;

};

#endif