#include "request.h"
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

Request::Request(int client_fd) {
    char buffer[16384];
    ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0) {
        close(client_fd);
        return;
    }

    buffer[bytes_received] = '\0';
    std::string request(buffer);

    size_t request_line_end = request.find("\r\n");
    size_t headers_end = request.find("\r\n\r\n");

    if (request_line_end == std::string::npos || headers_end == std::string::npos) {
        return;
    }
    std::string request_line = request.substr(0, request_line_end);

    size_t first_space = request_line.find(' ');
    size_t second_space = request_line.find(' ', first_space + 1);

    method = request_line.substr(0, first_space);
    path = request_line.substr(first_space + 1, second_space - (first_space + 1));
    http_version = request_line.substr(second_space + 1);

    // Parse query parameters from path (/users?id=42&active=true)
    size_t query_start = path.find('?');
    if (query_start != std::string::npos) {
        std::string query_string = path.substr(query_start + 1);
        path = path.substr(0, query_start);  // Remove query string from path

        size_t pos = 0;
        while (pos < query_string.size()) {
            size_t amp_pos = query_string.find('&', pos);
            if (amp_pos == std::string::npos) amp_pos = query_string.size();

            std::string pair = query_string.substr(pos, amp_pos - pos);
            size_t eq_pos = pair.find('=');

            if (eq_pos != std::string::npos) {
                query[pair.substr(0, eq_pos)] = pair.substr(eq_pos + 1);
            }

            pos = amp_pos + 1;
        }
    }

    std::string headers_section = request.substr(request_line_end + 2,
                                                    headers_end - (request_line_end + 2));

    size_t pos = 0;
    size_t next_line;
    while ((next_line = headers_section.find("\r\n", pos)) != std::string::npos) {
        std::string line = headers_section.substr(pos, next_line - pos);

        size_t colon_pos = line.find(": ");
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 2);
            headers[key] = value;
        }

        pos = next_line + 2;
    }

    if (pos < headers_section.size()) {
        std::string line = headers_section.substr(pos);
        size_t colon_pos = line.find(": ");
        if (colon_pos != std::string::npos) {
            headers[line.substr(0, colon_pos)] = line.substr(colon_pos + 2);
        }
    }

    if (headers_end + 4 < request.size()) {
        body = request.substr(headers_end + 4);
    }

    if (headers.count("Content-Length")) {
        int content_length = std::stoi(headers["Content-Length"]);

        while (body.size() < content_length) {
            char temp_buffer[4096];
            ssize_t bytes = recv(client_fd, temp_buffer, sizeof(temp_buffer), 0);
            if (bytes <= 0) break;
            body.append(temp_buffer, bytes);
        }
    }
}
