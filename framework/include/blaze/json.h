#ifndef BLAZE_JSON_WRAPPER_H
#define BLAZE_JSON_WRAPPER_H

#include <boost/json.hpp>
#include <string>
#include <memory>
#include <variant>
#include <optional>

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

    // Json({{"key", val}})
    Json(std::initializer_list<std::pair<std::string_view, boost::json::value_ref>> list);

    // Helper to detect if T is a pair-like type compatible with our object constructor
    template<typename T>
    struct is_object_pair : std::false_type {};

    template<typename K, typename V>
    struct is_object_pair<std::pair<K, V>> : std::is_convertible<K, std::string_view> {};

    // Array Constructor: Json({1, 2, 3})
    // Enabled ONLY if T is NOT a pair compatible with the Object constructor
    template<typename T, typename = std::enable_if_t<!is_object_pair<T>::value>>
    Json(std::initializer_list<T> list) {
        boost::json::array arr;
        for(const auto& item : list) {
            arr.push_back(boost::json::value_from(item));
        }
        data_.type = Type::BOOST_VAL;
        data_.boost_ptr = std::make_shared<boost::json::value>(std::move(arr));
    }
    
    template<typename... Args>
    static Json array(Args&&... args) {
        boost::json::array arr;
        arr.reserve(sizeof...(args));
        (arr.push_back(boost::json::value_from(std::forward<Args>(args))), ...);
        return Json(boost::json::value(std::move(arr)));
    }

    Json operator[](size_t idx) const;
    Json operator[](int idx) const { return (*this)[static_cast<size_t>(idx)]; }
    Json operator[](std::string_view key) const;
    Json operator[](const char* key) const { return (*this)[std::string_view(key)]; }

    bool has(std::string_view key) const;

    template<typename T>
    std::optional<T> try_get(std::string_view key) const {
        if (!has(key)) return std::nullopt;
        // For now, we rely on as<T> being robust or implicit conversion
        // Ideally we check if conversion is valid.
        // Let's rely on existence first.
        return (*this)[key].as<T>();
    }

    template<typename T>
    void set(std::string_view key, T&& value) {
        if (data_.type == Type::BOOST_VAL && data_.boost_ptr->is_object()) {
            data_.boost_ptr->as_object().insert_or_assign(key, boost::json::value_from(std::forward<T>(value)));
        }
    }

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

    friend Json tag_invoke(boost::json::value_to_tag<Json>, const boost::json::value& jv) {
        return Json(jv);
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
template<> Json Json::as<Json>() const;

} // namespace blaze

#endif