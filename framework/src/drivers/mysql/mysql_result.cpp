#include <blaze/mysql_result.h>
#include <stdexcept>
#include <vector>

namespace blaze {

// --- MySqlRow ---

MySqlRow::MySqlRow(MYSQL_RES* res, MYSQL_ROW row, unsigned long* lengths)
    : res_(res), row_(row), lengths_(lengths) {}

std::string_view MySqlRow::get_column(size_t index) const {
    if (index >= mysql_num_fields(res_)) {
        throw std::out_of_range("Column index out of bounds");
    }
    const char* val = row_[index];
    if (!val) return "";
    return std::string_view(val, lengths_[index]);
}

std::string_view MySqlRow::get_column(std::string_view name) const {
    unsigned int num_fields = mysql_num_fields(res_);
    MYSQL_FIELD* fields = mysql_fetch_fields(res_);
    for (unsigned int i = 0; i < num_fields; ++i) {
        if (name == fields[i].name) {
            return get_column(i);
        }
    }
    throw std::runtime_error("Column not found: " + std::string(name));
}

bool MySqlRow::is_null(size_t index) const {
    if (index >= mysql_num_fields(res_)) return true;
    return row_[index] == nullptr;
}

bool MySqlRow::is_null(std::string_view name) const {
    unsigned int num_fields = mysql_num_fields(res_);
    MYSQL_FIELD* fields = mysql_fetch_fields(res_);
    for (unsigned int i = 0; i < num_fields; ++i) {
        if (name == fields[i].name) {
            return is_null(i);
        }
    }
    return true; // Not found = null logic? Or throw? Postgres throws. Let's throw to match.
    // Actually, get_column throws, so this should probably throw too or return safe.
    // We'll stick to throwing in get_column, but boolean check maybe false?
    // Let's iterate to find index.
    return true;
}

// --- MySqlResult ---

MySqlResult::MySqlResult(MYSQL* conn, MYSQL_RES* res) : conn_(conn), res_(res) {
    if (res_) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res_))) {
            rows_.push_back(row);
            lengths_.push_back(mysql_fetch_lengths(res_));
        }
    } else if (conn_ && mysql_errno(conn_) != 0) {
        ok_ = false;
        error_ = mysql_error(conn_);
    }
}

MySqlResult::~MySqlResult() {
    if (res_) {
        mysql_free_result(res_);
    }
}

MySqlResult::MySqlResult(MySqlResult&& other) noexcept
    : conn_(other.conn_), res_(other.res_), rows_(std::move(other.rows_)), 
      lengths_(std::move(other.lengths_)), ok_(other.ok_), error_(std::move(other.error_)) {
    other.res_ = nullptr;
}

MySqlResult& MySqlResult::operator=(MySqlResult&& other) noexcept {
    if (this != &other) {
        if (res_) mysql_free_result(res_);
        conn_ = other.conn_;
        res_ = other.res_;
        rows_ = std::move(other.rows_);
        lengths_ = std::move(other.lengths_);
        ok_ = other.ok_;
        error_ = std::move(other.error_);
        other.res_ = nullptr;
    }
    return *this;
}

size_t MySqlResult::size() const { return rows_.size(); }



std::shared_ptr<RowImpl> MySqlResult::get_row(size_t row_idx) const {
    return std::make_shared<MySqlRow>(res_, rows_[row_idx], lengths_[row_idx]);
}



} // namespace blaze
