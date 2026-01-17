#include <blaze/mysql_result.h>
#include <stdexcept>
#include <vector>

namespace blaze {

MySqlField::MySqlField(const char* data, unsigned long len) : data_(data), len_(len) {}

bool MySqlField::is_null() const { return data_ == nullptr; }

template<> std::string MySqlField::as<std::string>() const {
    if (is_null()) return "";
    return std::string(data_, len_);
}

template<> int MySqlField::as<int>() const {
    if (is_null()) return 0;
    return std::stoi(data_);
}

// --- Row ---

MySqlRow::MySqlRow(MYSQL_RES* res, MYSQL_ROW row, unsigned long* lengths)
    : res_(res), row_(row), lengths_(lengths) {}

MySqlField MySqlRow::operator[](int col_idx) const {
    return {row_[col_idx], lengths_[col_idx]};
}

MySqlField MySqlRow::operator[](const char* col_name) const {
    unsigned int num_fields = mysql_num_fields(res_);
    MYSQL_FIELD* fields = mysql_fetch_fields(res_);
    for (unsigned int i = 0; i < num_fields; ++i) {
        if (std::string_view(col_name) == fields[i].name) {
            return (*this)[i];
        }
    }
    throw std::runtime_error("Column not found: " + std::string(col_name));
}

// --- Iterator ---

MySqlRowIterator::MySqlRowIterator(MySqlResult* parent, int row_idx) 
    : parent_(parent), row_idx_(row_idx) {}

MySqlRow MySqlRowIterator::operator*() const {
    return (*parent_)[row_idx_];
}

MySqlRowIterator& MySqlRowIterator::operator++() {
    row_idx_++;
    return *this;
}

bool MySqlRowIterator::operator!=(const MySqlRowIterator& other) const {
    return row_idx_ != other.row_idx_;
}

// --- Result ---

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
bool MySqlResult::empty() const { return rows_.empty(); }

MySqlRow MySqlResult::operator[](size_t row_idx) const {
    return {res_, rows_[row_idx], lengths_[row_idx]};
}

MySqlRowIterator MySqlResult::begin() { return {this, 0}; }
MySqlRowIterator MySqlResult::end() { return {this, (int)size()}; }

} // namespace blaze
