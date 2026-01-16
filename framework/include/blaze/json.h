#ifndef BLAZE_JSON_WRAPPER_H
#define BLAZE_JSON_WRAPPER_H

#include <boost/json.hpp>
#include <string>
#include <string_view>
#include <stdexcept>

namespace blaze {

class Json {
private:
    boost::json::value value_;
    boost::json::value* ptr_ = nullptr; // If set, we are a reference/view

    // Helper to get the underlying value ptr
    boost::json::value* get() const {
        return ptr_ ? ptr_ : const_cast<boost::json::value*>(&value_);
    }

public:
    Json() : value_(nullptr) {}
    Json(boost::json::value v) : value_(std::move(v)) {}
    Json(boost::json::value* v) : ptr_(v) {} // Reference constructor


    Json operator[](std::string_view key) const {
        auto* v = get();
        if (!v->is_object()) throw std::runtime_error("JSON is not an object");

        return Json( &v->as_object().at(key) );
    }


    Json operator[](const char* key) const {
        return operator[](std::string_view(key));
    }

    template<typename T>
    T as() const {
        return boost::json::value_to<T>(*get());
    }

    operator std::string() const {
        return as<std::string>();
    }

    operator int() const {
        return as<int>();
    }

    operator bool() const {
        return as<bool>();
    }

    bool contains(std::string_view key) const {
        auto* v = get();
        return v->is_object() && v->as_object().contains(key);
    }

    boost::json::value& raw() { return *get(); }
    const boost::json::value& raw() const { return *get(); }
};

} // namespace blaze

#endif
