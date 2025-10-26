#include <sys/socket.h>   // socket(), bind(), listen(), accept(), send(), recv()
#include <netinet/in.h>   // sockaddr_in, htons(), htonl(), INADDR_ANY
#include <arpa/inet.h>    // inet_ntop(), inet_pton()
#include <unistd.h>       // close()
#include <iostream>       // std::cout, std::cerr
#include <cstring>        // memset(), strerror()
#include <sstream>
#include <fstream>
#include <string>
#include <chrono>
#include "thread_pool.h"
#include "logger.h"
#include <atomic>
#include <signal.h>
#include "framework/request.h"
#include "framework/response.h"

std::atomic<bool> server_running{true};
int server_fd = -1;  // Global so signal handler can access

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

void handle_shutdown(const int sig) {
    std::cout << "\nShutdown signal received (signal " << sig << ")\n";
    server_running = false;
    if (server_fd != -1) {
        shutdown(server_fd, SHUT_RDWR);  // Force accept() to fail
        close(server_fd);
    }
}

void handle_client(int client_fd, const std::string& client_ip, Logger& logger) {
    auto start_time = std::chrono::steady_clock::now();

    Request req(client_fd);
    Response res;

    if (req.path == "/") {
        req.path = "/index.html";
    }

    if (req.path.find("..") != std::string::npos) {
        res.status(403).send("403 Forbidden\n");
        res.send_to_client(client_fd);

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();

        logger.log_access(client_ip, req.method, req.path, res.get_status(), elapsed);
        close(client_fd);
        return;

    }


    std::string filepath = "../public" + req.path;
    std::ifstream file(filepath, std::ios::binary);

    if (file.is_open()) {
        std::stringstream ss;
        ss << file.rdbuf();
        res.status(200)
           .header("Content-Type", get_mime_type(filepath))
           .send(ss.str());
    } else {
        res.status(404).send("404 Not Found\n");
    }

    res.send_to_client(client_fd);

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start_time).count();

    logger.log_access(client_ip, req.method, req.path, res.get_status(), elapsed);
    close(client_fd);
}

int main() {

    Logger logger;
    signal(SIGINT, handle_shutdown);
    signal(SIGTERM, handle_shutdown);

    size_t num_threads = std::thread::hardware_concurrency();
    ThreadPool pool(num_threads);

    server_fd = socket(AF_INET, SOCK_STREAM, 0); // Open TCP socket

    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    std::cout << "Server starting on port 8080 with " << num_threads << " worker threads\n";
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

    while (server_running) {
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);

        if (!server_running) break;  // Check flag after accept returns

        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        std::string client_ip_str(client_ip);

        pool.enqueue([client_fd, client_ip_str, &logger]() {
            handle_client(client_fd, client_ip_str, logger);
        });
    }
    std::cout << "Server stopped. Cleaning up...\n";
    return 0;
}