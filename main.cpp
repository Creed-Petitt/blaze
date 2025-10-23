#include <sys/socket.h>   // socket(), bind(), listen(), accept(), send(), recv()
#include <netinet/in.h>   // sockaddr_in, htons(), htonl(), INADDR_ANY
#include <arpa/inet.h>    // inet_ntop(), inet_pton()
#include <unistd.h>       // close()
#include <iostream>       // std::cout, std::cerr
#include <cstring>        // memset(), strerror()
#include <sstream>
#include <fstream>
#include <map>
#include <string>

int main() {

    // Create routing table
    std::map<std::string, std::string> routes;
    routes["/api/status"] = "status.json";
    routes["/api/users"] = "users.json";
    routes["/api/metrics"] = "metrics.json";
    routes["/api/config"] = "config.json";
    routes["/health"] = "health.txt";
    routes["/info"] = "info.txt";
    routes["/data"] = "data.csv";

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

        char buffer[4096];

        ssize_t bytes_reveived = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        buffer[bytes_reveived] = '\0';

        std::string request(buffer);

        size_t request_line_end = request.find("\r\n");
        std::string request_line = request.substr(0, request_line_end);

        size_t first_space = request_line.find(' ');
        size_t second_space = request_line.find(' ', first_space + 1);
        size_t third_space = request_line.find(' ', second_space + 1);

        std::string method = request_line.substr(0, first_space);
        std::string path = request_line.substr(first_space + 1, second_space - (first_space + 1));
        std::string http_ver = request_line.substr(second_space + 1, third_space - (second_space + 1));

        std::string response;

        if (routes.find(path) != routes.end()) {
            std::string filename = routes[path];

            //Try to open file
            std::ifstream file("../public/" + filename);

            if (file.is_open()) {
                std::string content((std::istreambuf_iterator<char>(file)),
                                      std::istreambuf_iterator<char>());
                file.close();

                 std::string content_type = "text/plain";

                if (filename.find("json") != std::string::npos) {
                    content_type = "application/json";
                } else if (filename.find("csv") != std::string::npos) {
                    content_type = "text/csv";
                }

                // Build HTTP response
                response = "HTTP/1.1 200 OK\r\n";
                response += "Content-Type: " + content_type + "\r\n";
                response += "Content-Length: " + std::to_string(content.size()) + "\r\n";
                response += "\r\n";
                response += content;

            } else {

                std::string body = "500 Internal Server Error\n";
                response = "HTTP/1.1 500 Internal Server Error\r\n";
                response += "Content-Type: text/plain\r\n";
                response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
                response += "\r\n";
                response += body;
            }
        } else {

            std::string body = "404 Not Found\n";
            response = "HTTP/1.1 404 Not Found\r\n";
            response += "Content-Type: text/plain\r\n";
            response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
            response += "\r\n";
            response += body;
        }
        if (bytes_reveived == 0) {
            perror("recv");
            close(client_fd);
            continue;
        }
        ssize_t bytes_sent = send(client_fd, response.c_str(), response.size(), 0);
        if (bytes_sent == -1) {
            perror("send");
            close(client_fd);
            close(server_fd);
            return 1;
        }
        close(client_fd);
    }
    return 0;
}