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

class PyroServer {

public:
    explicit PyroServer(int port);

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

    struct Connection {
        int fd;
        std::string read_buffer;   // Incoming HTTP data
        std::string write_buffer;  // Outgoing HTTP response
    };

    std::unordered_map<int, Connection> connections;

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
};


#endif