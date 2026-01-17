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
    const boost::json::value* ptr_ = nullptr;

    // Helper to get the underlying value ptr
    const boost::json::value* get() const {
        return ptr_ ? ptr_ : &value_;
    }

public:
    Json() : value_(nullptr) {}
    Json(boost::json::value v) : value_(std::move(v)) {}
    Json(const boost::json::value* v) : ptr_(v) {} // Const pointer constructor


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

    const boost::json::value& raw() const { return *get(); }

    class Iterator {
        const boost::json::value* ptr_;
        bool is_array_;
        size_t idx_;
    public:
        Iterator(const boost::json::value* v, size_t idx) : ptr_(v), idx_(idx) {
            is_array_ = v->is_array();
        }
        
        Json operator*() const {
            if (is_array_) return Json( &ptr_->as_array().at(idx_) );
            return Json(); // Should not happen for non-arrays in loop
        }

        Iterator& operator++() {
            idx_++;
            return *this;
        }

        bool operator!=(const Iterator& other) const {
            return idx_ != other.idx_;
        }
    };

    Iterator begin() const {
        auto* v = get();
        if (v->is_array()) return Iterator(v, 0);
        return Iterator(v, 0); // Empty iterator if not array
    }

    Iterator end() const {
        auto* v = get();
        if (v->is_array()) return Iterator(v, v->as_array().size());
        return Iterator(v, 0);
    }

    size_t size() const {
        auto* v = get();
        if (v->is_array()) return v->as_array().size();
        if (v->is_object()) return v->as_object().size();
        return 0;
    }

    bool empty() const { return size() == 0; }
};

} // namespace blaze

#endif
