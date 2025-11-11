#include "HttpServer.h"
#include "app.h"
#include "request.h"
#include "response.h"
#include <arpa/inet.h>
#include <atomic>
#include <cctype>
#include <memory>
#include <algorithm>

// Maximum request body size (100 MB) - hard limit to prevent OOM attacks
constexpr size_t MAX_REQUEST_BODY_SIZE = 100 * 1024 * 1024;

namespace {
    std::atomic<size_t> local_connection_counter{0};
}


int HttpServer::create_listening_socket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw std::runtime_error("failed to create server socket");
    }

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(fd);
        throw std::runtime_error("setsockopt SO_REUSEADDR failed");
    }

    make_socket_non_blocking(fd);

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(fd, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) < 0) {
        close(fd);
        throw std::runtime_error("failed to bind server socket");
    }

    if (listen(fd, BACKLOG) < 0) {
        close(fd);
        throw std::runtime_error("failed to listen");
    }

    std::cout << "Server socket created (non-blocking, fd="
              << fd << ")" << std::endl;
    return fd;
}

HttpServer::HttpServer(int port,
                         App* app,
                         int existing_server_fd,
                         bool owns_listener,
                         std::atomic<size_t>* active_connections,
                         size_t max_connections)
    : port(port),
      running(false),
      shutdown_started_(false),
      server_fd(existing_server_fd),
      epoll_fd(-1),
      owns_listener_(owns_listener),
      active_connections_(active_connections ? active_connections : &local_connection_counter),
      max_connections_(max_connections ? max_connections : MAX_CONNECTIONS),
      app_(app) {

    std::cout << "Initializing event-driven server on port "
                << port << std::endl;

    if (server_fd < 0) {
        server_fd = create_listening_socket(port);
        owns_listener_ = true;
    } else if (owns_listener_ && server_fd >= 0) {
        // ensure listener is non-blocking even if provided
        make_socket_non_blocking(server_fd);
    }

    setup_epoll();
}

HttpServer::~HttpServer() {
    shutdown();
}

void HttpServer::run() {
    running.store(true, std::memory_order_release);
    epoll_event events[MAX_EVENTS];

    std::cout << "Event loop starting..." << std::endl;

    time_t last_cleanup = time(nullptr);

    while (running.load(std::memory_order_acquire)) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, 1);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "epoll_wait error: " << strerror(errno) << std::endl;
            break;
        }

        if (!running.load(std::memory_order_acquire)) {
            break;
        }

        // Process response queue FIRST before handling new events
        process_response_queue();

        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            uint32_t evt = events[i].events;

            try {
                if (fd == server_fd) {
                    handle_new_connection();
                }
                else if (evt & EPOLLIN) {
                    handle_readable(fd);
                }
                else if (evt & EPOLLOUT) {
                    handle_writable(fd);
                }
            } catch (const std::exception& e) {
                std::cerr << "Exception in event handler for fd=" << fd << ": " << e.what() << std::endl;
                if (fd != server_fd) {
                    close_connection(fd);
                }
            }
        }

        time_t now = time(nullptr);
        if (now - last_cleanup > 60) {
            cleanup_stale_connections(300);
            last_cleanup = now;
        }
    }

    std::cout << "Event loop stopped" << std::endl;
}

void HttpServer::shutdown() {
    bool expected = false;
    if (!shutdown_started_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return;
    }

    running.store(false, std::memory_order_release);
    std::cout << "Shutting down server..." << std::endl;

    // Close all client connections
    while (!connections.empty()) {
        int fd = connections.begin()->first;
        close_connection_unlocked(fd);
    }

    // Close epoll and server socket
    if (epoll_fd >= 0) {
        close(epoll_fd);
        epoll_fd = -1;
    }

    if (server_fd >= 0 && owns_listener_) {
        close(server_fd);
        server_fd = -1;
    }
}

void HttpServer::stop() {
    running.store(false, std::memory_order_release);
}

void HttpServer::make_socket_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        throw std::runtime_error("could not get the file descriptor flag");
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        throw std::runtime_error("could not set the file descriptor flag");
    }
}

void HttpServer::epoll_add(int fd, uint32_t events) {
    epoll_event ev = {};
    ev.events = events;
    ev.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        std::cerr << "epoll_add failed for fd " << fd << std::endl;
    }
}

void HttpServer::epoll_modify(int fd, uint32_t events) {
    epoll_event ev = {};
    ev.events = events;
    ev.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) < 0) {
        std::cerr << "epoll_modify failed for fd " << fd << std::endl;
    }
}

void HttpServer::epoll_remove(int fd) {

    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr) < 0) {
        std::cerr << "epoll_remove failed for fd " << fd << std::endl;
    }
}

void HttpServer::setup_epoll() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        throw std::runtime_error("epoll_create1 failed");
    }

    // Add server socket to epoll (watch for incoming connections)
    uint32_t events = EPOLLIN;

    events |= EPOLLEXCLUSIVE;

    epoll_add(server_fd, events);

    std::cout << " Epoll initialized (fd=" << epoll_fd << ")" << std::endl;
}

void HttpServer::close_connection(int fd) {
    close_connection_unlocked(fd);
}

void HttpServer::close_connection_unlocked(int fd) {
    auto it = connections.find(fd);
    if (it == connections.end()) {
        return;
    }

    connections.erase(it);
    epoll_remove(fd);
    close(fd);
    if (active_connections_) {
        active_connections_->fetch_sub(1, std::memory_order_acq_rel);
    }
}

void HttpServer::handle_new_connection() {
    while (true) {
        sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);

        int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr *>(&client_address), &client_len);

        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            if (errno == EBADF || errno == EINVAL || errno == ENOTSOCK) {
                return;
            }
            std::cerr << "[Server] Accept error: " << strerror(errno) << std::endl;
            break;
        }

        size_t current = active_connections_->fetch_add(1, std::memory_order_acq_rel);
        if (current >= max_connections_) {
            active_connections_->fetch_sub(1, std::memory_order_acq_rel);
            std::cerr << "[Server] Max connections (" << max_connections_
                      << ") reached, rejecting new connection (fd=" << client_fd << ")" << std::endl;
            close(client_fd);
            continue;
        }

        try {
            make_socket_non_blocking(client_fd);
        } catch (const std::exception& e) {
            std::cerr << "[Server] Failed to set non-blocking mode for fd=" << client_fd
                      << ": " << e.what() << std::endl;
            active_connections_->fetch_sub(1, std::memory_order_acq_rel);
            close(client_fd);
            continue;
        }

        epoll_add(client_fd, EPOLLIN | EPOLLET);

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_address.sin_addr, ip_str, INET_ADDRSTRLEN);

        connections[client_fd] = Connection{};
        connections[client_fd].fd = client_fd;
        connections[client_fd].client_ip = ip_str;
        connections[client_fd].last_activity = time(nullptr);

        // std::cout << "[Server] New connection: fd=" << client_fd << std::endl;
    }
}

void HttpServer::handle_readable(int fd) {
    auto it = connections.find(fd);
    if (it == connections.end()) {
        return;
    }

    Connection& conn = it->second;
    char buffer[4096];

    while (true) {
        int bytes = read(fd, buffer, sizeof(buffer));

        if (bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            std::cerr << "[Server] Read error on fd=" << fd << ": "
                      << strerror(errno) << std::endl;
            close_connection_unlocked(fd);
            return;
        } else if (bytes == 0) {
            close_connection_unlocked(fd);
            return;
        } else {
            if (conn.read_buffer.size() + bytes > MAX_REQUEST_BODY_SIZE) {
                std::cerr << "[Server] Request body too large for fd=" << fd
                          << " (current: " << conn.read_buffer.size()
                          << " bytes, max: " << MAX_REQUEST_BODY_SIZE << " bytes)" << std::endl;
                close_connection_unlocked(fd);
                return;
            }

            conn.read_buffer.append(buffer, bytes);
            conn.last_activity = time(nullptr);
        }
    }

    std::string& buff = conn.read_buffer;

    size_t headers_end = buff.find("\r\n\r\n");
    if (headers_end == std::string::npos) {
        // Only timeout incomplete requests waiting for headers
        time_t now = time(nullptr);

        if (buff.size() > 8192) {
            send_error_response(fd, 400, "Bad Request");
            close_connection_unlocked(fd);
            return;
        }
        return;
    }

    if (headers_end > 8192) {
        send_error_response(fd, 400, "Bad Request");
        close_connection_unlocked(fd);
        return;
    }

    // Use shared helper from Request class - no duplication!
    auto content_length_opt = Request::extract_content_length(buff, headers_end, MAX_REQUEST_BODY_SIZE);

    if (!content_length_opt.has_value()) {
        // Invalid Content-Length or exceeds max size
        send_error_response(fd, 413, "Payload Too Large");
        close_connection_unlocked(fd);
        return;
    }

    size_t content_length = *content_length_opt;
    size_t required_bytes = headers_end + 4 + content_length;
    if (buff.size() < required_bytes) {
        return;
    }

    Request req(buff);

    if (req.method.empty()) {
        send_error_response(fd, 400, "Bad Request");
        close_connection_unlocked(fd);
        return;
    }

    bool keep_alive = true;
    if (req.http_version == "HTTP/1.0") {
        keep_alive = false;
    }

    if (req.has_header("Connection")) {
        std::string conn_header = req.get_header("Connection");
        std::transform(conn_header.begin(), conn_header.end(), conn_header.begin(), ::tolower);
        if (conn_header == "close") {
            keep_alive = false;
        } else if (conn_header == "keep-alive") {
            keep_alive = true;
        }
    }

    conn.keep_alive = keep_alive;

    int client_fd = fd;
    bool client_keep_alive = keep_alive;
    std::string client_ip = conn.client_ip;

    bool dispatched = app_->dispatch_async([this, client_fd, req, client_keep_alive, client_ip]() {
        auto response = std::make_unique<std::string>(
            app_->handle_request(const_cast<Request&>(req), client_ip, client_keep_alive));
        std::lock_guard lock(response_queue_mutex_);
        response_queue_.push(PendingResponse{client_fd, std::move(response)});
    });

    if (!dispatched) {
        // Thread pool queue is full - send 503 and close connection
        send_error_response(fd, 503, "Service Unavailable");
        close_connection_unlocked(fd);
        return;
    }

    auto it_after = connections.find(fd);
    if (it_after != connections.end()) {
        // Only erase the bytes we consumed, keep any pipelined requests
        // DON'T process them now - wait until we send the response for THIS request first
        if (required_bytes < it_after->second.read_buffer.size()) {
            it_after->second.read_buffer.erase(0, required_bytes);
        } else {
            it_after->second.read_buffer.clear();
        }
    }
}

void HttpServer::handle_writable(int fd) {
    auto it = connections.find(fd);
    if (it == connections.end()) {
        return;
    }

    std::string& buf = it->second.write_buffer;

    // With EPOLLET, must write until EAGAIN or buffer is empty
    while (!buf.empty()) {
        ssize_t bytes = write(fd, buf.c_str(), buf.size());

        if (bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Socket buffer full, will get notified when writable again
                return;
            }
            close_connection_unlocked(fd);
            return;
        }

        buf.erase(0, bytes);
    }

    // All data written
    auto it_after = connections.find(fd);
    if (it_after == connections.end()) {
        return;
    }

    if (it_after->second.keep_alive) {
        // Don't clear read_buffer - may contain partial next request or pipelined data
        it_after->second.write_buffer.clear();
        it_after->second.last_activity = time(nullptr);
        epoll_modify(fd, EPOLLIN | EPOLLET);

        // If there's data in read_buffer, process it now
        if (!it_after->second.read_buffer.empty()) {
            handle_readable(fd);
        }
    }
    else {
        close_connection_unlocked(fd);
    }
}

void HttpServer::process_response_queue() {
    std::lock_guard lock(response_queue_mutex_);
    while (!response_queue_.empty()) {
        auto resp = std::move(response_queue_.front());
        response_queue_.pop();

        auto it = connections.find(resp.fd);
        if (it != connections.end() && resp.response) {
            it->second.write_buffer = std::move(*resp.response);
            epoll_modify(resp.fd, EPOLLOUT | EPOLLET);
        }

        // std::cout << "Response ready to send on fd=" << resp.fd << std::endl;
    }
}

void HttpServer::cleanup_stale_connections(int timeout_seconds) {
    time_t now = time(nullptr);
    std::vector<int> to_close;

    for (auto& [fd, conn] : connections) {
        if (now - conn.last_activity > timeout_seconds) {
            to_close.push_back(fd);
        }
    }

    for (int fd : to_close) {
        close_connection_unlocked(fd);
    }
}

void HttpServer::send_error_response(int fd, int status_code, const std::string& message) {
    Response res;
    res.status(status_code).json({{"error", message}});

    auto it = connections.find(fd);
    if (it != connections.end()) {
        it->second.write_buffer = res.build_response();
        epoll_modify(fd, EPOLLOUT | EPOLLET);
    }
}
