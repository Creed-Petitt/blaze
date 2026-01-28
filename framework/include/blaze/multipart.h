#ifndef BLAZE_MULTIPART_H
#define BLAZE_MULTIPART_H

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <optional>

namespace blaze {

/**
 * @brief Represents a single part of a multipart/form-data request.
 * Could be a file or a regular form field.
 */
struct MultipartPart {
    std::string name;                // Form field name
    std::string filename;            // Original filename (if it's a file)
    std::string content_type;        // MIME type (e.g. image/jpeg)
    std::string_view data;           // Raw content (view into request body)

    bool is_file() const { return !filename.empty(); }
    size_t size() const { return data.size(); }

    /**
     * @brief Saves the part data to a file on disk.
     */
    bool save_to(const std::string& path) const;
};

/**
 * @brief Container for all parsed multipart data.
 */
class MultipartFormData {
public:
    void add_part(MultipartPart part) {
        parts_.push_back(std::move(part));
    }

    const std::vector<MultipartPart>& parts() const { return parts_; }

    /**
     * @brief Gets a form field value by name.
     */
    std::optional<std::string_view> get_field(const std::string& name) const {
        for (const auto& part : parts_) {
            if (!part.is_file() && part.name == name) return part.data;
        }
        return std::nullopt;
    }

    /**
     * @brief Gets all files from the form.
     */
    std::vector<const MultipartPart*> files() const {
        std::vector<const MultipartPart*> result;
        for (const auto& part : parts_) {
            if (part.is_file()) result.push_back(&part);
        }
        return result;
    }

    /**
     * @brief Gets a specific file by field name.
     */
    const MultipartPart* get_file(const std::string& name) const {
        for (const auto& part : parts_) {
            if (part.is_file() && part.name == name) return &part;
        }
        return nullptr;
    }

private:
    std::vector<MultipartPart> parts_;
};

namespace multipart {
    MultipartFormData parse(std::string_view body, std::string_view boundary);
}

} // namespace blaze

#endif
