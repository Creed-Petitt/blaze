#ifndef HTTP_SERVER_RESPONSE_H
#define HTTP_SERVER_RESPONSE_H

#include <string>
#include <unordered_map>

class Response {
private:
    int status_code_ = 0;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;

    std::string build_response() const;
    static std::string get_status_text(const int code);

public:
    Response();

    Response& status(const int code);
    Response& header(const std::string& key, const std::string& value);
    Response& send(const std::string& text);

    void send_to_client(const int client_fd) const;
    int get_status() const;

};

#endif