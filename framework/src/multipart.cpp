#include <blaze/multipart.h>
#include <fstream>
#include <algorithm>

namespace blaze {

bool MultipartPart::save_to(const std::string& path) const {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    file.write(data.data(), data.size());
    return true;
}

namespace multipart {

static std::string_view trim(std::string_view s) {
    auto first = s.find_first_not_of(" \t\r\n");
    if (std::string_view::npos == first) return "";
    auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, (last - first + 1));
}

MultipartFormData parse(std::string_view body, std::string_view boundary) {
    MultipartFormData result;
    if (boundary.empty()) return result;

    std::string start_boundary = "--" + std::string(boundary);
    std::string end_boundary = start_boundary + "--";

    size_t pos = 0;
    while (true) {
        pos = body.find(start_boundary, pos);
        if (pos == std::string_view::npos) break;

        // Check if it's the end boundary
        if (body.substr(pos, end_boundary.size()) == end_boundary) break;

        pos += start_boundary.size();
        
        // Move past \r\n
        if (pos + 2 <= body.size() && body[pos] == '\r' && body[pos+1] == '\n') pos += 2;
        else if (pos + 1 <= body.size() && body[pos] == '\n') pos += 1;

        // Find the start of the data (after headers, marked by \r\n\r\n)
        size_t header_end = body.find("\r\n\r\n", pos);
        if (header_end == std::string_view::npos) {
            header_end = body.find("\n\n", pos);
            if (header_end == std::string_view::npos) break;
        }

        std::string_view headers = body.substr(pos, header_end - pos);
        size_t data_start = header_end + (body[header_end] == '\r' ? 4 : 2);

        // Find the next boundary (end of data)
        size_t data_end = body.find(start_boundary, data_start);
        if (data_end == std::string_view::npos) break;
        
        // Trim trailing \r\n before boundary
        if (data_end >= 2 && body[data_end-2] == '\r' && body[data_end-1] == '\n') data_end -= 2;
        else if (data_end >= 1 && body[data_end-1] == '\n') data_end -= 1;

        MultipartPart part;
        part.data = body.substr(data_start, data_end - data_start);

        size_t h_pos = 0;
        while (h_pos < headers.size()) {
            size_t next_h = headers.find('\n', h_pos);
            std::string_view line = (next_h == std::string_view::npos) 
                                    ? headers.substr(h_pos) 
                                    : headers.substr(h_pos, next_h - h_pos);
            
            if (line.find("Content-Disposition:") != std::string_view::npos) {
                auto name_pos = line.find("name=\"");
                if (name_pos != std::string_view::npos) {
                    name_pos += 6;
                    auto name_end = line.find("\"", name_pos);
                    part.name = std::string(line.substr(name_pos, name_end - name_pos));
                }
                auto file_pos = line.find("filename=\"");
                if (file_pos != std::string_view::npos) {
                    file_pos += 10;
                    auto file_end = line.find("\"", file_pos);
                    part.filename = std::string(line.substr(file_pos, file_end - file_pos));
                }
            } else if (line.find("Content-Type:") != std::string_view::npos) {
                auto type_pos = line.find(':') + 1;
                part.content_type = std::string(trim(line.substr(type_pos)));
            }

            if (next_h == std::string_view::npos) break;
            h_pos = next_h + 1;
        }

        result.add_part(std::move(part));
        pos = data_end;
    }

    return result;
}

} // namespace multipart
} // namespace blaze
