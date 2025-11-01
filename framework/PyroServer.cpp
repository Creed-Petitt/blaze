#include "PyroServer.h"
#include "app.h"
#include "request.h"
#include "response.h"

// Maximum request body size (100 MB) - hard limit to prevent OOM attacks
constexpr size_t MAX_REQUEST_BODY_SIZE = 100 * 1024 * 1024;


PyroServer::PyroServer(int port, App* app)
    : port(port),
      running(false),
      server_fd(-1),
      epoll_fd(-1),
      app_(app) {

    std::cout << "[Pyro] Initializing event-driven server on port "
                << port << std::endl;

    create_server_socket();
    setup_epoll();
}

PyroServer::~PyroServer() {
    shutdown();
}

void PyroServer::run() {
    running = true;
    epoll_event events[MAX_EVENTS];

    std::cout << "[Pyro] Event loop starting..." << std::endl;

    while (running) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, 1);  // 1ms timeout for fast response queue polling

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "[Pyro] epoll_wait error: " << strerror(errno) << std::endl;
            break;
        }

        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            uint32_t evt = events[i].events;

            if (fd == server_fd) {
                handle_new_connection();
            }
            else if (evt & EPOLLIN) {
                handle_readable(fd);
            }
            else if (evt & EPOLLOUT) {
                handle_writable(fd);
            }
        }
        process_response_queue();
    }

    std::cout << "[Pyro] Event loop stopped" << std::endl;
}

void PyroServer::shutdown() {
    std::cout << "[Pyro] Shutting down server..." << std::endl;
    running = false;

    // Close all client connections
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for (auto& [fd, conn] : connections) {
            close(fd);
        }
        connections.clear();
    }

    // Close epoll and server socket
    if (epoll_fd >= 0) {
        close(epoll_fd);
        epoll_fd = -1;
    }

    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }
}

void PyroServer::make_socket_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        throw std::runtime_error("could not get the file descriptor flag");
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        throw std::runtime_error("could not set the file descriptor flag");
    }
}

void PyroServer::epoll_add(int fd, uint32_t events) {
    epoll_event ev = {};
    ev.events = events;
    ev.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        std::cerr << "[Pyro] epoll_add failed for fd " << fd << std::endl;
    }
}

void PyroServer::epoll_modify(int fd, uint32_t events) {
    epoll_event ev = {};
    ev.events = events;
    ev.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) < 0) {
        std::cerr << "[Pyro] epoll_modify failed for fd " << fd << std::endl;
    }
}

void PyroServer::epoll_remove(int fd) {

    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr) < 0) {
        std::cerr << "[Pyro] epoll_remove failed for fd " << fd << std::endl;
    }
}

void PyroServer::create_server_socket() {
    // Open socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::runtime_error("failed to create server socket");
    }

    // Set socket options (reuse address + reuse port for multi-core)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("setsockopt SO_REUSEADDR failed");
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("setsockopt SO_REUSEPORT failed");
    }

    // Make non-blocking
    make_socket_non_blocking(server_fd);

    // Bind to address
    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind to port
    if (bind(server_fd, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) < 0) {
        throw std::runtime_error("failed to bind server socket");
    }

    // Listen
    if (listen(server_fd, BACKLOG) < 0) {
        throw std::runtime_error("failed to listen");
    }

    std::cout << "[Pyro] Server socket created (non-blocking, fd="
            << server_fd << ")" << std::endl;
}

void PyroServer::setup_epoll() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        throw std::runtime_error("epoll_create1 failed");
    }

    // Add server socket to epoll (watch for incoming connections)
    epoll_add(server_fd, EPOLLIN);

    std::cout << "[Pyro] Epoll initialized (fd=" << epoll_fd << ")" << std::endl;
}

void PyroServer::close_connection(int fd) {
    // std::cout << "[Pyro] Closing connection: fd=" << fd << std::endl;

    epoll_remove(fd);
    close(fd);

    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections.erase(fd);
    }
}

void PyroServer::handle_new_connection() {
    while (true) {
        sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);

        int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr *>(&client_address), &client_len);

        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            std::cerr << "[Pyro] Accept error: " << strerror(errno) << std::endl;
            break;
        }

        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            if (connections.size() >= MAX_CONNECTIONS) {
                std::cerr << "[Pyro] Max connections (" << MAX_CONNECTIONS
                          << ") reached, rejecting new connection (fd=" << client_fd << ")" << std::endl;
                close(client_fd);  // Immediately close the socket
                continue;  // Try to accept more (in case some are closing)
            }
        }

        make_socket_non_blocking(client_fd);

        epoll_add(client_fd, EPOLLIN);

        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            connections[client_fd] = Connection{};
            connections[client_fd].fd = client_fd;
        }

        // std::cout << "[Pyro] New connection: fd=" << client_fd << std::endl;
    }
}

void PyroServer::handle_readable(int fd) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    char buffer[4096];

    while (true) {
        int bytes = read(fd, buffer, sizeof(buffer));

        if (bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            std::cerr << "[Pyro] Read error on fd=" << fd << ": "
                            << strerror(errno) << std::endl;
            close_connection(fd);
            return;
        }
        else if (bytes == 0) {
            // std::cout << "[Pyro] Client disconnected: fd=" << fd << std::endl;
            close_connection(fd);
            return;
        }
        else {
            // Check if appending would exceed max body size (OOM protection)
            if (connections[fd].read_buffer.size() + bytes > MAX_REQUEST_BODY_SIZE) {
                std::cerr << "[Pyro] Request body too large for fd=" << fd
                          << " (current: " << connections[fd].read_buffer.size()
                          << " bytes, max: " << MAX_REQUEST_BODY_SIZE << " bytes)" << std::endl;
                close_connection(fd);
                return;
            }

            connections[fd].read_buffer.append(buffer, bytes);
            // std::cout << "[Pyro] Read " << bytes << " bytes from fd=" << fd
            //           << " (total: " << connections[fd].read_buffer.size() << ")" << std::endl;
        }
    }

    // Check if we have a complete HTTP request
    std::string& buff = connections[fd].read_buffer;

    size_t headers_end = buff.find("\r\n\r\n");
    if (headers_end == std::string::npos) {
        // std::cout << "[Pyro] Waiting for complete headers..." << std::endl;
        return;  // Wait for more data
    }

    size_t content_length = 0;
    size_t cl_pos = buff.find("Content-Length: ");

    if (cl_pos != std::string::npos && cl_pos < headers_end) {
        size_t value_start = cl_pos + 16;
        size_t value_end = buff.find("\r\n", value_start);

        try {
            std::string cl_str = buff.substr(value_start, value_end - value_start);

            if (cl_str.empty() || cl_str[0] == '-') {
                std::cerr << "[Pyro] Invalid Content-Length: negative or empty (fd=" << fd << ")" << std::endl;
                close_connection(fd);
                return;
            }

            long long cl_value = std::stoll(cl_str);

            if (cl_value < 0 || static_cast<unsigned long long>(cl_value) > MAX_REQUEST_BODY_SIZE) {
                std::cerr << "[Pyro] Content-Length too large: " << cl_value
                          << " bytes (max: " << MAX_REQUEST_BODY_SIZE << " bytes) for fd=" << fd << std::endl;
                close_connection(fd);
                return;
            }

            content_length = static_cast<size_t>(cl_value);

        } catch (const std::invalid_argument& e) {
            std::cerr << "[Pyro] Invalid Content-Length format (fd=" << fd << "): " << e.what() << std::endl;
            close_connection(fd);
            return;
        } catch (const std::out_of_range& e) {
            std::cerr << "[Pyro] Content-Length out of range (fd=" << fd << "): " << e.what() << std::endl;
            close_connection(fd);
            return;
        }
    }

    size_t required_bytes = headers_end + 4 + content_length;
    if (buff.size() < required_bytes) {
        // std::cout << "[Pyro] Waiting for body: have " << buff.size()
        //           << " bytes, need " << required_bytes << std::endl;
        return;  // Wait for more data
    }

    // Request is complete!
    // std::cout << "[Pyro] Complete HTTP request received (fd=" << fd
    //           << ", " << buff.size() << " bytes)" << std::endl;

    Request req(buff);

    bool keep_alive = true;
    if (req.http_version == "HTTP/1.0") {
        keep_alive = false;
    }

    if (req.has_header("Connection")) {
        std::string conn = req.get_header("Connection");
        std::transform(conn.begin(), conn.end(), conn.begin(), ::tolower);
        if (conn == "close") {
            keep_alive = false;
        }
        else if (conn == "keep-alive") {
            keep_alive = true;
        }
    }

    connections[fd].keep_alive = keep_alive;

    int client_fd = fd;
    bool client_keep_alive = keep_alive;  // Capture for lambda
    app_->dispatch_async([this, client_fd, req, client_keep_alive]() {
        // Call App's request handler (middleware + routing)

        {
            std::string response_str = app_->handle_request(const_cast<Request&>(req), "127.0.0.1", client_keep_alive);
            std::lock_guard lock(response_queue_mutex_);
            response_queue_.push(PendingResponse{client_fd, response_str});
        }

        // std::cout << "[Pyro] Response enqueued for fd=" << client_fd << std::endl;
    });
    connections[fd].read_buffer.clear();
}

void PyroServer::handle_writable(int fd) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    std::string& buf = connections[fd].write_buffer;

    if (buf.empty()) {
        return;
    }

    ssize_t bytes = write(fd, buf.c_str(), buf.size());

    if (bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        close_connection(fd);
        return;
    }

    buf.erase(0,bytes);

    if (buf.empty()) {
        if (connections[fd].keep_alive) {
            connections[fd].read_buffer.clear();
            connections[fd].write_buffer.clear();
            connections[fd].last_activity = time(nullptr);
            epoll_modify(fd, EPOLLIN);
        }
        else {
            close_connection(fd);
        }
    }
}

void PyroServer::process_response_queue() {
    while (true) {
        PendingResponse resp;
        {
            std::lock_guard lock(response_queue_mutex_);
            if (response_queue_.empty()) break;
            resp = std::move(response_queue_.front());
            response_queue_.pop();
        }

        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            if (connections.count(resp.fd)) {
                connections[resp.fd].write_buffer = std::move(resp.response);
            }
        }

        epoll_modify(resp.fd, EPOLLOUT);

        // std::cout << "[Pyro] Response ready to send on fd=" << resp.fd << std::endl;
    }
}