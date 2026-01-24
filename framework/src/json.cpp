#include <blaze/json.h>

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
    if (data_.type == Type::BOOST_VAL) {
        if (data_.boost_ptr->is_array()) return data_.boost_ptr->as_array().size();
        if (data_.boost_ptr->is_object()) return data_.boost_ptr->as_object().size();
    }
    return 0;
}

Json Json::operator[](size_t idx) const {
    if (data_.type == Type::BOOST_VAL && data_.boost_ptr->is_array()) {
        return Json(data_.boost_ptr->as_array()[idx]);
    }
    return Json();
}

Json Json::operator[](std::string_view key) const {
    if (data_.type == Type::BOOST_VAL && data_.boost_ptr->is_object()) {
        if (data_.boost_ptr->as_object().contains(key)) {
            return Json(data_.boost_ptr->as_object().at(key));
        }
    }
    return Json();
}

template<>
std::string Json::as<std::string>() const {
    if (data_.type == Type::BOOST_VAL) {
        if (data_.boost_ptr->is_string()) return std::string(data_.boost_ptr->as_string());
        if (data_.boost_ptr->is_int64()) return std::to_string(data_.boost_ptr->as_int64());
        if (data_.boost_ptr->is_double()) return std::to_string(data_.boost_ptr->as_double());
        if (data_.boost_ptr->is_bool()) return data_.boost_ptr->as_bool() ? "true" : "false";
    }
    return "";
}

template<>
int Json::as<int>() const {
    if (data_.type == Type::BOOST_VAL && data_.boost_ptr->is_int64()) {
        return (int)data_.boost_ptr->as_int64();
    }
    std::string s = as<std::string>();
    try { return std::stoi(s); } catch (...) { return 0; }
}

Json::operator std::string() const { return as<std::string>(); }
Json::operator int() const { return as<int>(); }

Json::operator boost::json::value() const {
    if (data_.type == Type::BOOST_VAL) return *data_.boost_ptr;
    return boost::json::value(as<std::string>());
}

} // namespace blaze
