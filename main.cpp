#include <sys/socket.h>   // socket(), bind(), listen(), accept(), send(), recv()
#include <netinet/in.h>   // sockaddr_in, htons(), htonl(), INADDR_ANY
#include <arpa/inet.h>    // inet_ntop(), inet_pton()
#include <unistd.h>       // close()
#include <iostream>       // std::cout, std::cerr
#include <cstring>        // memset(), strerror()
#include <sstream>
#include <fstream>
#include <string>
#include "thread_pool.h"

std::string get_mime_type(const std::string& path) {
    if (path.find(".json") != std::string::npos) return "application/json";
    if (path.find(".html") != std::string::npos) return "text/html";
    if (path.find(".css") != std::string::npos) return "text/css";
    if (path.find(".js") != std::string::npos) return "application/javascript";
    if (path.find(".txt") != std::string::npos) return "text/plain";
    if (path.find(".csv") != std::string::npos) return "text/csv";
    if (path.find(".png") != std::string::npos) return "image/png";
    if (path.find(".jpg") != std::string::npos || path.find(".jpeg") != std::string::npos) return "image/jpeg";
    return "application/octet-stream";
}

void handle_client(int client_fd, const std::string& client_ip) {
    char buffer[4096];

    ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0) {
        if (bytes_received == -1) {
            perror("recv");
        }
        close(client_fd);
        return;
    }

    buffer[bytes_received] = '\0';

    std::string request(buffer);

    size_t request_line_end = request.find("\r\n");
    if (request_line_end == std::string::npos) {
        std::string body = "400 Bad Request\n";
        std::string response = "HTTP/1.1 400 Bad Request\r\n";
        response += "Content-Type: text/plain\r\n";
        response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        response += "\r\n";
        response += body;

        send(client_fd, response.c_str(), response.size(), 0);
        close(client_fd);
        return;
    }
    std::string request_line = request.substr(0, request_line_end);

    size_t first_space = request_line.find(' ');
    size_t second_space = request_line.find(' ', first_space + 1);

    if (first_space == std::string::npos || second_space == std::string::npos) {
        std::string body = "400 Bad Request\n";
        std::string response = "HTTP/1.1 400 Bad Request\r\n";
        response += "Content-Type: text/plain\r\n";
        response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        response += "\r\n";
        response += body;

        send(client_fd, response.c_str(), response.size(), 0);
        close(client_fd);
        return;
    }

    std::string method = request_line.substr(0, first_space);
    std::string path = request_line.substr(first_space + 1, second_space - (first_space + 1));
    std::string http_ver = request_line.substr(second_space + 1);

    if (path == "/") {
        path = "/index.html";
    }

    if (path.find("..") != std::string::npos) {
        std::string body = "403 Forbidden\n";
        std::string response = "HTTP/1.1 403 Forbidden\r\n";
        response += "Content-Type: text/plain\r\n";
        response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        response += "\r\n";
        response += body;

        send(client_fd, response.c_str(), response.size(), 0);
        close(client_fd);
        return;
    }

    // Construct file path
    std::string filepath = "../public" + path;

    // Try to open and serve the file
    std::ifstream file(filepath, std::ios::binary);
    std::string response;

    if (file.is_open()) {
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        file.close();

        std::string content_type = get_mime_type(filepath);

        // Build HTTP response
        response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: " + content_type + "\r\n";
        response += "Content-Length: " + std::to_string(content.size()) + "\r\n";
        response += "\r\n";
        response += content;
    } else {
        std::string body = "404 Not Found\n";
        response = "HTTP/1.1 404 Not Found\r\n";
        response += "Content-Type: text/plain\r\n";
        response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        response += "\r\n";
        response += body;
    }

    ssize_t bytes_sent = send(client_fd, response.c_str(), response.size(), 0);
    if (bytes_sent == -1) {
        perror("send");
    }
    close(client_fd);
}

int main() {

    size_t num_threads = std::thread::hardware_concurrency();
    ThreadPool pool(num_threads);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0); // Open TCP socket

    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    std::cout << "Socket created successfully (fd=" << server_fd << ")\n";

    int yes = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt");
        return 1;
    }

    sockaddr_in server_addr {};

    server_addr.sin_family = AF_INET; //IPv4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(8080);

    if (bind(server_fd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 128) == -1) {
        perror("listen");
        return 1;
    }

    sockaddr_in client_addr {};
    socklen_t client_len = sizeof(client_addr);

    while (true) {
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);

        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        std::string client_ip_str(client_ip);

        pool.enqueue([client_fd, client_ip_str]() {
            handle_client(client_fd, client_ip_str);
        });
    }
    return 0;
}