#include <blaze/request.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <cctype>

namespace {
    std::string to_lower(const std::string& input) {
        std::string result = input;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return result;
    }

    std::string trim(const std::string& input) {
        size_t first = 0;
        size_t last = input.size();

        while (first < input.size() && std::isspace(static_cast<unsigned char>(input[first]))) {
            ++first;
        }

        while (last > first && std::isspace(static_cast<unsigned char>(input[last - 1]))) {
            --last;
        }

        return input.substr(first, last - first);
    }
}

// Static helper to parse Content-Length value (shared by both HttpServer and Request constructor)
size_t Request::parse_content_length_value(const std::string& value) {
    // Trim whitespace
    size_t start = value.find_first_not_of(" \t");
    size_t end = value.find_last_not_of(" \t");

    if (start == std::string::npos || end == std::string::npos) {
        throw std::invalid_argument("Empty Content-Length value");
    }

    std::string trimmed = value.substr(start, end - start + 1);

    unsigned long long cl = std::stoull(trimmed);

    return static_cast<size_t>(cl);
}

std::optional<size_t> Request::extract_content_length(
    const std::string& buffer,
    size_t headers_end,
    size_t max_size
) {

    std::string headers_lower = buffer.substr(0, headers_end);
    std::transform(headers_lower.begin(), headers_lower.end(),
                   headers_lower.begin(), ::tolower);

    size_t cl_pos = headers_lower.find("content-length:");
    if (cl_pos == std::string::npos) {
        return 0;  // No Content-Length header
    }

    // Find the value
    size_t value_start = cl_pos + 15;  // length of "content-length:"
    size_t line_end = buffer.find("\r\n", value_start);

    if (line_end == std::string::npos || line_end > headers_end) {
        line_end = headers_end;
    }

    std::string value = buffer.substr(value_start, line_end - value_start);

    try {
        size_t content_length = parse_content_length_value(value);

        // Validate against max size
        if (content_length > max_size) {
            return std::nullopt;  // Exceeds max size
        }

        return content_length;

    } catch (const std::exception&) {
        return std::nullopt;  // Invalid Content-Length
    }
}

Request::Request(std::string& raw_http) {
    size_t request_line_end = raw_http.find("\r\n");
    size_t headers_end = raw_http.find("\r\n\r\n");

    if (request_line_end == std::string::npos || headers_end == std::string::npos) {
        return;
    }
    std::string request_line = raw_http.substr(0, request_line_end);

    size_t first_space = request_line.find(' ');
    size_t second_space = request_line.find(' ', first_space + 1);

    if (first_space == std::string::npos || second_space == std::string::npos) {
        return;
    }

    method = request_line.substr(0, first_space);
    path = request_line.substr(first_space + 1, second_space - (first_space + 1));
    http_version = request_line.substr(second_space + 1);

    if (method != "GET" && method != "POST" && method != "PUT" &&
        method != "DELETE" && method != "PATCH" && method != "OPTIONS" && method != "HEAD") {
        return;
    }

    if (http_version != "HTTP/1.0" && http_version != "HTTP/1.1") {
        return;
    }

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

    std::string headers_section = raw_http.substr(request_line_end + 2,
                                                    headers_end - (request_line_end + 2));

    size_t pos = 0;
    size_t next_line;
    while ((next_line = headers_section.find("\r\n", pos)) != std::string::npos) {
        std::string line = headers_section.substr(pos, next_line - pos);

        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = trim(line.substr(0, colon_pos));
            std::string value = trim(line.substr(colon_pos + 1));
            headers[to_lower(key)] = value;
        }

        pos = next_line + 2;
    }

    if (pos < headers_section.size()) {
        std::string line = headers_section.substr(pos);
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = trim(line.substr(0, colon_pos));
            std::string value = trim(line.substr(colon_pos + 1));
            headers[to_lower(key)] = value;
        }
    }

    size_t body_start = headers_end + 4;
    if (body_start < raw_http.size()) {
        // Use the same helper HttpServer uses - no duplication!
        auto content_length_opt = extract_content_length(raw_http, headers_end);

        if (content_length_opt.has_value() && *content_length_opt > 0) {
            size_t content_length = *content_length_opt;
            size_t available = raw_http.size() - body_start;
            size_t to_copy = std::min(content_length, available);
            body = raw_http.substr(body_start, to_copy);
        }
    }
}

nlohmann::json Request::json() const {
    try {
        return nlohmann::json::parse(body);
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("Invalid JSON in request body: " + std::string(e.what()));
    }
}

// Helper methods for safe parameter and header access
std::string Request::get_query(const std::string& key, const std::string& default_val) const {
    auto it = query.find(key);
    if (it != query.end()) {
        return it->second;
    }
    return default_val;
}

int Request::get_query_int(const std::string& key, int default_val) const {
    auto it = query.find(key);
    if (it != query.end()) {
        try {
            return std::stoi(it->second);
        } catch (const std::exception&) {
            return default_val;
        }
    }
    return default_val;
}

std::string Request::get_header(const std::string& key, const std::string& default_val) const {
    std::string normalized = to_lower(key);
    auto it = headers.find(normalized);
    if (it != headers.end()) {
        return it->second;
    }
    return default_val;
}

bool Request::has_header(const std::string& key) const {
    return headers.find(to_lower(key)) != headers.end();
}

std::optional<int> Request::get_param_int(const std::string& key) const {
    auto it = params.find(key);
    if (it != params.end()) {
        try {
            return std::stoi(it->second);
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}
