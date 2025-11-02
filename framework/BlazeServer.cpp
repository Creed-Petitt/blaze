#include "BlazeServer.h"
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


int BlazeServer::create_listening_socket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw std::runtime_error("failed to create server socket");
    }

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(fd);
        throw std::runtime_error("setsockopt SO_REUSEADDR failed");
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        close(fd);
        throw std::runtime_error("setsockopt SO_REUSEPORT failed");
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

    std::cout << "[Pyro] Server socket created (non-blocking, fd="
              << fd << ")" << std::endl;
    return fd;
}

BlazeServer::BlazeServer(int port,
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

    std::cout << "[Pyro] Initializing event-driven server on port "
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

BlazeServer::~BlazeServer() {
    shutdown();
}

void BlazeServer::run() {
    running.store(true, std::memory_order_release);
    epoll_event events[MAX_EVENTS];

    std::cout << "[Pyro] Event loop starting..." << std::endl;

    time_t last_cleanup = time(nullptr);

    while (running.load(std::memory_order_acquire)) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, 1);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "[Pyro] epoll_wait error: " << strerror(errno) << std::endl;
            break;
        }

        if (!running.load(std::memory_order_acquire)) {
            break;
        }

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
                std::cerr << "[Pyro] Exception in event handler for fd=" << fd << ": " << e.what() << std::endl;
                if (fd != server_fd) {
                    close_connection(fd);
                }
            }
        }
        process_response_queue();

        time_t now = time(nullptr);
        if (now - last_cleanup > 60) {
            cleanup_stale_connections(300);
            last_cleanup = now;
        }
    }

    std::cout << "[Pyro] Event loop stopped" << std::endl;
}

void BlazeServer::shutdown() {
    bool expected = false;
    if (!shutdown_started_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return;
    }

    running.store(false, std::memory_order_release);
    std::cout << "[Pyro] Shutting down server..." << std::endl;

    // Close all client connections
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        while (!connections.empty()) {
            int fd = connections.begin()->first;
            close_connection_unlocked(fd);
        }
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

void BlazeServer::stop() {
    running.store(false, std::memory_order_release);
}

void BlazeServer::make_socket_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        throw std::runtime_error("could not get the file descriptor flag");
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        throw std::runtime_error("could not set the file descriptor flag");
    }
}

void BlazeServer::epoll_add(int fd, uint32_t events) {
    epoll_event ev = {};
    ev.events = events;
    ev.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        std::cerr << "[Pyro] epoll_add failed for fd " << fd << std::endl;
    }
}

void BlazeServer::epoll_modify(int fd, uint32_t events) {
    epoll_event ev = {};
    ev.events = events;
    ev.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) < 0) {
        std::cerr << "[Pyro] epoll_modify failed for fd " << fd << std::endl;
    }
}

void BlazeServer::epoll_remove(int fd) {

    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr) < 0) {
        std::cerr << "[Pyro] epoll_remove failed for fd " << fd << std::endl;
    }
}

void BlazeServer::setup_epoll() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        throw std::runtime_error("epoll_create1 failed");
    }

    // Add server socket to epoll (watch for incoming connections)
    uint32_t events = EPOLLIN;
#ifdef EPOLLEXCLUSIVE
    events |= EPOLLEXCLUSIVE;
#endif
    epoll_add(server_fd, events);

    std::cout << "[Pyro] Epoll initialized (fd=" << epoll_fd << ")" << std::endl;
}

void BlazeServer::close_connection(int fd) {
    // std::cout << "[Pyro] Closing connection: fd=" << fd << std::endl;

    std::lock_guard<std::mutex> lock(connections_mutex_);
    close_connection_unlocked(fd);
}

void BlazeServer::close_connection_unlocked(int fd) {
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

void BlazeServer::handle_new_connection() {
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
            std::cerr << "[Pyro] Accept error: " << strerror(errno) << std::endl;
            break;
        }

        size_t current = active_connections_->fetch_add(1, std::memory_order_acq_rel);
        if (current >= max_connections_) {
            active_connections_->fetch_sub(1, std::memory_order_acq_rel);
            std::cerr << "[Pyro] Max connections (" << max_connections_
                      << ") reached, rejecting new connection (fd=" << client_fd << ")" << std::endl;
            close(client_fd);
            continue;
        }

        try {
            make_socket_non_blocking(client_fd);
        } catch (const std::exception& e) {
            std::cerr << "[Pyro] Failed to set non-blocking mode for fd=" << client_fd
                      << ": " << e.what() << std::endl;
            active_connections_->fetch_sub(1, std::memory_order_acq_rel);
            close(client_fd);
            continue;
        }

        epoll_add(client_fd, EPOLLIN);

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_address.sin_addr, ip_str, INET_ADDRSTRLEN);

        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            connections[client_fd] = Connection{};
            connections[client_fd].fd = client_fd;
            connections[client_fd].client_ip = ip_str;
            connections[client_fd].last_activity = time(nullptr);
        }

        // std::cout << "[Pyro] New connection: fd=" << client_fd << std::endl;
    }
}

void BlazeServer::handle_readable(int fd) {
    std::unique_lock<std::mutex> lock(connections_mutex_);

    auto it = connections.find(fd);
    if (it == connections.end()) {
        return;
    }

    Connection& conn = it->second;
    char buffer[4096];

    while (true) {
        lock.unlock();
        int bytes = read(fd, buffer, sizeof(buffer));
        lock.lock();

        if (bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            std::cerr << "[Pyro] Read error on fd=" << fd << ": "
                      << strerror(errno) << std::endl;
            close_connection_unlocked(fd);
            return;
        } else if (bytes == 0) {
            close_connection_unlocked(fd);
            return;
        } else {
            if (conn.read_buffer.size() + bytes > MAX_REQUEST_BODY_SIZE) {
                std::cerr << "[Pyro] Request body too large for fd=" << fd
                          << " (current: " << conn.read_buffer.size()
                          << " bytes, max: " << MAX_REQUEST_BODY_SIZE << " bytes)" << std::endl;
                close_connection_unlocked(fd);
                return;
            }

            conn.read_buffer.append(buffer, bytes);
            conn.last_activity = time(nullptr);
        }
    }

    time_t now = time(nullptr);
    if (now - conn.last_activity > 30) {
        send_error_response(fd, 408, "Request Timeout");
        close_connection_unlocked(fd);
        return;
    }

    std::string& buff = conn.read_buffer;

    size_t headers_end = buff.find("\r\n\r\n");
    if (headers_end == std::string::npos) {
        if (buff.size() > 8192) {
            send_error_response(fd, 400, "Bad Request");
            close_connection_unlocked(fd);
        }
        return;
    }

    if (headers_end > 8192) {
        send_error_response(fd, 400, "Bad Request");
        close_connection_unlocked(fd);
        return;
    }

    size_t content_length = 0;
    size_t header_pos = 0;

    while (header_pos < headers_end) {
        size_t line_end = buff.find("\r\n", header_pos);
        if (line_end == std::string::npos || line_end > headers_end) {
            line_end = headers_end;
        }

        std::string line = buff.substr(header_pos, line_end - header_pos);
        size_t colon_pos = line.find(':');

        if (colon_pos != std::string::npos) {
            std::string name = line.substr(0, colon_pos);
            while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back()))) {
                name.pop_back();
            }

            std::string value = line.substr(colon_pos + 1);
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
                value.erase(value.begin());
            }
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
                value.pop_back();
            }

            std::transform(name.begin(), name.end(), name.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            if (name == "content-length") {
                try {
                    if (value.empty() || value[0] == '-') {
                        std::cerr << "[Pyro] Invalid Content-Length: negative or empty (fd=" << fd << ")" << std::endl;
                        send_error_response(fd, 400, "Bad Request");
                        close_connection_unlocked(fd);
                        return;
                    }

                    long long cl_value = std::stoll(value);

                    if (cl_value < 0 || static_cast<unsigned long long>(cl_value) > MAX_REQUEST_BODY_SIZE) {
                        std::cerr << "[Pyro] Content-Length too large: " << cl_value
                                  << " bytes (max: " << MAX_REQUEST_BODY_SIZE << " bytes) for fd=" << fd << std::endl;
                        send_error_response(fd, 413, "Payload Too Large");
                        close_connection_unlocked(fd);
                        return;
                    }

                    content_length = static_cast<size_t>(cl_value);
                } catch (const std::invalid_argument& e) {
                    std::cerr << "[Pyro] Invalid Content-Length format (fd=" << fd << "): " << e.what() << std::endl;
                    send_error_response(fd, 400, "Bad Request");
                    close_connection_unlocked(fd);
                    return;
                } catch (const std::out_of_range& e) {
                    std::cerr << "[Pyro] Content-Length out of range (fd=" << fd << "): " << e.what() << std::endl;
                    send_error_response(fd, 400, "Bad Request");
                    close_connection_unlocked(fd);
                    return;
                }

                break;
            }
        }

        if (line_end == headers_end) {
            break;
        }
        header_pos = line_end + 2;
    }

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

    {
        std::lock_guard<std::mutex> queue_lock(response_queue_mutex_);
        if (response_queue_.size() >= 900) {
            send_error_response(fd, 503, "Server Busy");
            close_connection_unlocked(fd);
            return;
        }
    }

    lock.unlock();
    app_->dispatch_async([this, client_fd, req, client_keep_alive, client_ip]() {
        auto response = std::make_unique<std::string>(
            app_->handle_request(const_cast<Request&>(req), client_ip, client_keep_alive));
        std::lock_guard lock(response_queue_mutex_);
        if (response_queue_.size() < 1000) {
            response_queue_.push(PendingResponse{client_fd, std::move(response)});
        }
    });
    lock.lock();

    auto it_after = connections.find(fd);
    if (it_after != connections.end()) {
        it_after->second.read_buffer.clear();
    }
}

void BlazeServer::handle_writable(int fd) {
    std::unique_lock<std::mutex> lock(connections_mutex_);

    auto it = connections.find(fd);
    if (it == connections.end()) {
        return;
    }

    std::string& buf = it->second.write_buffer;

    if (buf.empty()) {
        return;
    }

    lock.unlock();
    ssize_t bytes = write(fd, buf.c_str(), buf.size());
    lock.lock();

    if (bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        close_connection_unlocked(fd);
        return;
    }

    buf.erase(0, bytes);

    if (buf.empty()) {
        auto it_after = connections.find(fd);
        if (it_after == connections.end()) {
            return;
        }

        if (it_after->second.keep_alive) {
            it_after->second.read_buffer.clear();
            it_after->second.write_buffer.clear();
            it_after->second.last_activity = time(nullptr);
            epoll_modify(fd, EPOLLIN);
            return;
        }
        else {
            close_connection_unlocked(fd);
            return;
        }
    }
}

void BlazeServer::process_response_queue() {
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
            auto it = connections.find(resp.fd);
            if (it != connections.end() && resp.response) {
                it->second.write_buffer = std::move(*resp.response);
                epoll_modify(resp.fd, EPOLLOUT);
            }
        }

        // std::cout << "[Pyro] Response ready to send on fd=" << resp.fd << std::endl;
    }
}

void BlazeServer::cleanup_stale_connections(int timeout_seconds) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
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

void BlazeServer::send_error_response(int fd, int status_code, const std::string& message) {
    Response res;
    res.status(status_code).json({{"error", message}});
    std::string response_str = res.build_response();

    ssize_t sent = 0;
    size_t total = response_str.size();
    while (sent < static_cast<ssize_t>(total)) {
        ssize_t n = write(fd, response_str.c_str() + sent, total - sent);
        if (n <= 0) break;
        sent += n;
    }
}
