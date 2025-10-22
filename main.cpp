#include <sys/socket.h>   // socket(), bind(), listen(), accept(), send(), recv()
#include <netinet/in.h>   // sockaddr_in, htons(), htonl(), INADDR_ANY
#include <arpa/inet.h>    // inet_ntop(), inet_pton()
#include <unistd.h>       // close()
#include <iostream>       // std::cout, std::cerr
#include <cstring>        // memset(), strerror()

int main() {

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {
        std::cout << "Socket failed with error code: " << std::strerror(errno) << std::endl;
        return 1;
    }

    std::cout << "Socket created successfully (fd=" << server_fd << ")\n";

    close (server_fd);

    return 42;
}