#include <blaze/json.h>
#include <blaze/exceptions.h>
#include <string>
#include <stdexcept>

namespace blaze {

Json::Json() {}

Json::Json(boost::json::value v) {
    data_.type = Type::BOOST_VAL;
    data_.boost_ptr = std::make_shared<boost::json::value>(std::move(v));
}

Json::Json(std::initializer_list<std::pair<std::string_view, boost::json::value_ref>> list) {
    boost::json::object obj;
    for (const auto& [key, value] : list) {
        obj.emplace(key, boost::json::value(value));
    }
    data_.type = Type::BOOST_VAL;
    data_.boost_ptr = std::make_shared<boost::json::value>(std::move(obj));
}

bool Json::is_ok() const {
    return data_.type != Type::NONE;
}

size_t Json::size() const {
    if (data_.type == Type::BOOST_VAL && data_.boost_ptr) {
        if (data_.boost_ptr->is_array()) return data_.boost_ptr->as_array().size();
        if (data_.boost_ptr->is_object()) return data_.boost_ptr->as_object().size();
    }
    return 0;
}

Json Json::operator[](size_t idx) const {
    if (data_.type == Type::BOOST_VAL && data_.boost_ptr && data_.boost_ptr->is_array()) {
        if (idx < data_.boost_ptr->as_array().size()) {
            return Json(data_.boost_ptr->as_array()[idx]);
        }
    }
    return Json();
}

Json Json::operator[](std::string_view key) const {
    if (data_.type == Type::BOOST_VAL && data_.boost_ptr && data_.boost_ptr->is_object()) {
        if (data_.boost_ptr->as_object().contains(key)) {
            return Json(data_.boost_ptr->as_object().at(key));
        }
    }
    return Json();
}

bool Json::has(std::string_view key) const {
    if (data_.type == Type::BOOST_VAL && data_.boost_ptr && data_.boost_ptr->is_object()) {
        return data_.boost_ptr->as_object().contains(key);
    }
    return false;
}

template<>
std::string Json::as<std::string>() const {
    if (data_.type == Type::BOOST_VAL && data_.boost_ptr) {
        if (data_.boost_ptr->is_string()) return std::string(data_.boost_ptr->as_string());
        if (data_.boost_ptr->is_int64()) return std::to_string(data_.boost_ptr->as_int64());
        if (data_.boost_ptr->is_uint64()) return std::to_string(data_.boost_ptr->as_uint64());
        if (data_.boost_ptr->is_double()) return std::to_string(data_.boost_ptr->as_double());
        if (data_.boost_ptr->is_bool()) return data_.boost_ptr->as_bool() ? "true" : "false";
        if (data_.boost_ptr->is_null()) return "";
    }
    return "";
}

template<>
int Json::as<int>() const {
    if (data_.type == Type::BOOST_VAL && data_.boost_ptr) {
        if (data_.boost_ptr->is_int64()) return (int)data_.boost_ptr->as_int64();
        if (data_.boost_ptr->is_uint64()) return (int)data_.boost_ptr->as_uint64();
    }
    
    std::string s = as<std::string>();
    if (s.empty()) return 0;
    
    try { 
        size_t pos;
        int val = std::stoi(s, &pos);
        if (pos != s.size()) throw std::invalid_argument("trailing characters");
        return val;
    } catch (...) { 
        throw BadRequest("Invalid integer format: " + s); 
    }
}

template<>
Json Json::as<Json>() const {
    return *this;
}

Json::operator std::string() const { return as<std::string>(); }
Json::operator int() const { return as<int>(); }

Json::operator boost::json::value() const {
    if (data_.type == Type::BOOST_VAL && data_.boost_ptr) return *data_.boost_ptr;
    return nullptr;
}

const boost::json::value& Json::value() const {
    if (data_.type == Type::BOOST_VAL && data_.boost_ptr) return *data_.boost_ptr;
    static const boost::json::value empty_val = nullptr;
    return empty_val;
}

boost::json::value Json::move_value() {
    if (data_.type == Type::BOOST_VAL && data_.boost_ptr) {
        return *data_.boost_ptr; 
    }
    return nullptr;
}

std::string Json::dump() const {
    if (data_.type == Type::BOOST_VAL && data_.boost_ptr) {
        return boost::json::serialize(*data_.boost_ptr);
    }
    return "null";
}

} // namespace blaze
