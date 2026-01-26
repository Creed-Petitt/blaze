#ifndef BLAZE_JSON_WRAPPER_H
#define BLAZE_JSON_WRAPPER_H

#include <boost/json.hpp>
#include <string>
#include <memory>
#include <variant>

namespace blaze {

class Json {
public:
    enum class Type { 
        NONE, 
        BOOST_VAL
    };

    Json();
    Json(boost::json::value v);

    // Generic constructor to allow Json({ ... }) syntax universally
    template<typename T>
    Json(const T& value) : Json(boost::json::value_from(value)) {}

    // Use value_ref to allow implicit conversion from std::string in initializer lists
    Json(std::initializer_list<std::pair<std::string_view, boost::json::value_ref>> list);

    Json operator[](size_t idx) const;
    Json operator[](int idx) const { return (*this)[static_cast<size_t>(idx)]; }
    Json operator[](std::string_view key) const;
    Json operator[](const char* key) const { return (*this)[std::string_view(key)]; }

    template<typename T>
    T as() const;

    explicit operator std::string() const;
    explicit operator int() const;

    operator boost::json::value() const;

    size_t size() const;
    bool empty() const { return size() == 0; }
    bool is_ok() const;

    std::string dump() const;

    // Enable Boost.JSON serialization support for vector<blaze::Json>
    friend void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Json& wrapper) {
        if (wrapper.data_.type == Type::BOOST_VAL && wrapper.data_.boost_ptr) {
            jv = *wrapper.data_.boost_ptr;
        } else {
            jv = nullptr;
        }
    }

private:
    struct Internal {
        Type type = Type::NONE;
        std::shared_ptr<boost::json::value> boost_ptr;
    };

    Internal data_;
    Json(Internal data) : data_(std::move(data)) {}
};

template<> std::string Json::as<std::string>() const;
template<> int Json::as<int>() const;

} // namespace blaze

#endif