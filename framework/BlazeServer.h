#ifndef HTTP_SERVER_BLAZESERVER_H
#define HTTP_SERVER_BLAZESERVER_H

#include <sys/epoll.h>      // epoll_create, epoll_ctl, epoll_wait
#include <sys/socket.h>     // socket, bind, listen, accept
#include <netinet/in.h>     // sockaddr_in
#include <fcntl.h>          // fcntl, O_NONBLOCK
#include <unistd.h>         // close, read, write
#include <cstring>          // memset
#include <iostream>         // logging
#include <unordered_map>    // connection tracking
#include <memory>
#include <string>
#include <queue>
#include <atomic>
#include <mutex>

class App;

class BlazeServer {

public:
    BlazeServer(int port, App* app, int server_fd = -1, bool owns_listener = true);

    ~BlazeServer();

    void run();

    void shutdown();
    void stop();

    static int create_listening_socket(int port);

private:
    int port;
    std::atomic<bool> running;

    int server_fd;
    int epoll_fd;
    bool owns_listener_;

    static const int MAX_EVENTS = 1024;
    static const int BACKLOG = 512;
    static const size_t MAX_CONNECTIONS = 10000;  // Maximum concurrent connections

    struct PendingResponse {
        int fd;
        std::unique_ptr<std::string> response;
    };

    struct Connection {
        int fd;
        std::string read_buffer;
        std::string write_buffer;
        std::string client_ip;
        bool keep_alive = true;
        time_t last_activity = 0;
    };

    std::unordered_map<int, Connection> connections;
    std::mutex connections_mutex_;  // Protects connections map access
    std::queue<PendingResponse> response_queue_;
    std::mutex response_queue_mutex_;

    App* app_;

    // Setup methods
    void setup_epoll();
    static void make_socket_non_blocking(int fd);

    // Epoll helpers
    void epoll_add(int fd, uint32_t events);
    void epoll_modify(int fd, uint32_t events);
    void epoll_remove(int fd);

    // Event handlers
    void handle_new_connection();
    void handle_readable(int fd);
    void handle_writable(int fd);
    void close_connection(int fd);
    void close_connection_unlocked(int fd);
    void cleanup_stale_connections(int timeout_seconds);
    void send_error_response(int fd, int status_code, const std::string& message);

    void process_response_queue();
};


#endif
