#include <blaze/json.h>
#include <blaze/pg_result.h>
#include <blaze/mysql_result.h>
#include <mysql.h>

namespace blaze {

Json::Json() {}

Json::Json(boost::json::value v) {
    data_.type = Type::BOOST_VAL;
    data_.boost_ptr = std::make_shared<boost::json::value>(std::move(v));
}

Json::Json(std::shared_ptr<PgResult> res) {
    data_.type = Type::PG_RESULT;
    data_.pg_ptr = std::move(res);
}

Json::Json(std::shared_ptr<MySqlResult> res) {
    data_.type = Type::MYSQL_RESULT;
    data_.mysql_ptr = std::move(res);
}

bool Json::is_ok() const {
    if (data_.type == Type::PG_RESULT) return data_.pg_ptr->is_ok();
    if (data_.type == Type::MYSQL_RESULT) return data_.mysql_ptr->is_ok();
    return data_.type != Type::NONE;
}

size_t Json::size() const {
    switch (data_.type) {
        case Type::PG_RESULT: return data_.pg_ptr->size();
        case Type::MYSQL_RESULT: return data_.mysql_ptr->size();
        case Type::BOOST_VAL: {
            if (data_.boost_ptr->is_array()) return data_.boost_ptr->as_array().size();
            if (data_.boost_ptr->is_object()) return data_.boost_ptr->as_object().size();
            return 0;
        }
        default: return 0;
    }
}

Json Json::operator[](size_t idx) const {
    Internal next = data_;
    if (data_.type == Type::PG_RESULT) {
        next.type = Type::PG_ROW;
        next.row = (int)idx;
        return Json(next);
    }
    if (data_.type == Type::MYSQL_RESULT) {
        next.type = Type::MYSQL_ROW;
        next.row = (int)idx;
        return Json(next);
    }
    if (data_.type == Type::BOOST_VAL && data_.boost_ptr->is_array()) {
        return Json(data_.boost_ptr->as_array()[idx]);
    }
    return Json();
}

Json Json::operator[](std::string_view key) const {
    Internal next = data_;
    if (data_.type == Type::PG_ROW) {
        next.type = Type::FIELD_PG;
        next.col = PQfnumber(data_.pg_ptr->get_raw(), std::string(key).c_str());
        return Json(next);
    }
    if (data_.type == Type::MYSQL_ROW) {
        next.type = Type::FIELD_MYSQL;
        MYSQL_RES* raw = data_.mysql_ptr->get_raw();
        int num_fields = mysql_num_fields(raw);
        MYSQL_FIELD* fields = mysql_fetch_fields(raw);
        for (int i = 0; i < num_fields; i++) {
            if (key == fields[i].name) {
                next.col = i;
                break;
            }
        }
        return Json(next);
    }
    if (data_.type == Type::BOOST_VAL && data_.boost_ptr->is_object()) {
        if (data_.boost_ptr->as_object().contains(key)) {
            return Json(data_.boost_ptr->as_object().at(key));
        }
    }
    return Json();
}

template<>
std::string Json::as<std::string>() const {
    if (data_.type == Type::FIELD_PG && data_.col != -1) {
        char* val = PQgetvalue(data_.pg_ptr->get_raw(), data_.row, data_.col);
        return val ? std::string(val) : "";
    }
    if (data_.type == Type::FIELD_MYSQL && data_.col != -1) {
        MYSQL_ROW row = data_.mysql_ptr->get_row_raw(data_.row);
        unsigned long* lens = data_.mysql_ptr->get_lengths_raw(data_.row);
        return row[data_.col] ? std::string(row[data_.col], lens[data_.col]) : "";
    }
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
