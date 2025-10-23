#include <sys/socket.h>   // socket(), bind(), listen(), accept(), send(), recv()
#include <netinet/in.h>   // sockaddr_in, htons(), htonl(), INADDR_ANY
#include <arpa/inet.h>    // inet_ntop(), inet_pton()
#include <unistd.h>       // close()
#include <iostream>       // std::cout, std::cerr
#include <cstring>        // memset(), strerror()

int main() {

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

    int client_fd = accept(server_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);

    if (client_fd == -1) {
        perror("accept");
        return 1;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    char buffer[4096];

    ssize_t bytes_reveived = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    buffer[bytes_reveived] = '\0';

    if (bytes_reveived == 0) {
        perror("recv");
        close(client_fd);
        close(server_fd);
        return 1;
    }

    const char* response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "Hello, World!";

    ssize_t bytes_sent = send(client_fd, response, strlen(response), 0);

    if (bytes_sent == -1) {
        perror("send");
        close(client_fd);
        close(server_fd);
        return 1;
    }

    close(client_fd);
    close (server_fd);

    return 0;
}