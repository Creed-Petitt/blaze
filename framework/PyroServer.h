#ifndef HTTP_SERVER_PYROSERVER_H
#define HTTP_SERVER_PYROSERVER_H

#include <sys/epoll.h>      // epoll_create, epoll_ctl, epoll_wait
#include <sys/socket.h>     // socket, bind, listen, accept
#include <netinet/in.h>     // sockaddr_in
#include <fcntl.h>          // fcntl, O_NONBLOCK
#include <unistd.h>         // close, read, write
#include <cstring>          // memset
#include <iostream>         // logging
#include <unordered_map>    // connection tracking
#include <string>
#include <string>
#include <queue>
#include <mutex>

class App;

class PyroServer {

public:
    PyroServer(int port, App* app);

    ~PyroServer();

    void run();

    void shutdown();

private:
    int port;
    bool running;

    int server_fd;
    int epoll_fd;

    static const int MAX_EVENTS = 1024;
    static const int BACKLOG = 512;
    static const size_t MAX_CONNECTIONS = 10000;  // Maximum concurrent connections

    struct PendingResponse {
        int fd;
        std::string response;
    };

    struct Connection {
        int fd;
        std::string read_buffer;   // Incoming HTTP data
        std::string write_buffer;  // Outgoing HTTP response
        bool keep_alive = true;
        time_t last_activity = 0;
    };

    std::unordered_map<int, Connection> connections;
    std::mutex connections_mutex_;  // Protects connections map access
    std::queue<PendingResponse> response_queue_;
    std::mutex response_queue_mutex_;

    App* app_;

    // Setup methods
    void create_server_socket();
    void setup_epoll();
    void make_socket_non_blocking(int fd);

    // Epoll helpers
    void epoll_add(int fd, uint32_t events);
    void epoll_modify(int fd, uint32_t events);
    void epoll_remove(int fd);

    // Event handlers
    void handle_new_connection();
    void handle_readable(int fd);
    void handle_writable(int fd);
    void close_connection(int fd);

    void process_response_queue();
};


#endif