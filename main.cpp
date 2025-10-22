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

    close (server_fd);

    return 0;
}