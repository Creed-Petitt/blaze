#include <blaze/pg_result.h>
#include <libpq-fe.h>
#include <stdexcept>

namespace blaze {

    PgField::PgField(const char* data, const int len) : data_(data), len_(len) {}

    bool PgField::is_null() const {
        return data_ == nullptr;
    }

    template<> std::string PgField::as<std::string>() const {
        if (is_null()) return "";
        return std::string(data_, len_);
    }

    template<> int PgField::as<int>() const {
        if (is_null()) return 0;
        return std::stoi(data_);
    }

    template<> float PgField::as<float>() const {
        if (is_null()) return 0.0f;
        return std::stof(data_);
    }

    template<> double PgField::as<double>() const {
        if (is_null()) return 0.0;
        return std::stod(data_);
    }

    template<> bool PgField::as<bool>() const {
        if (is_null()) return false;
        // Postgres returns 't' for true, 'f' for false
        return data_[0] == 't';
    }

    // ------------------------------------------------------------------------------

    PgRow::PgRow(PGresult* res, const int row_idx)
        : res_(res), row_idx_(row_idx) {}

    PgField PgRow::operator[](const int col_idx) const {
        if (col_idx < 0 || col_idx >= PQnfields(res_)) {
            throw std::out_of_range("Column index out of bounds");
        }

        if (PQgetisnull(res_, row_idx_, col_idx)) {
            return {nullptr, 0};
        }

        const char* val = PQgetvalue(res_, row_idx_, col_idx);
        const int len = PQgetlength(res_, row_idx_, col_idx);
        return {val, len};
    }

    PgField PgRow::operator[](const char* col_name) const {
        const int col_idx = PQfnumber(res_, col_name);
        if (col_idx == -1) {
            throw std::runtime_error("Column not found: " + std::string(col_name));
        }
        return (*this)[col_idx];
    }

    // ------------------------------------------------------------------------------

    PgRowIterator::PgRowIterator(PGresult* res, int row_idx) 
        : res_(res), row_idx_(row_idx) {}

    PgRow PgRowIterator::operator*() const {
        return {res_, row_idx_};
    }

    PgRowIterator& PgRowIterator::operator++() {
        ++row_idx_;
        return *this;
    }

    bool PgRowIterator::operator!=(const PgRowIterator& other) const {
        return row_idx_ != other.row_idx_;
    }

    // ------------------------------------------------------------------------------

    PgResult::PgResult(PGresult* res) : res_(res) {}

    PgResult::~PgResult() {
        if (res_) {
            PQclear(res_);
            res_ = nullptr;
        }
    }

    // Move Constructor
    PgResult::PgResult(PgResult&& other) noexcept : res_(other.res_) {
        other.res_ = nullptr;
    }

    // Move Assignment
    PgResult& PgResult::operator=(PgResult&& other) noexcept {
        if (this != &other) {
            if (res_) {
                PQclear(res_);
            }
            res_ = other.res_;
            other.res_ = nullptr;
        }
        return *this;
    }

    size_t PgResult::size() const {
        return res_ ? PQntuples(res_) : 0;
    }

    bool PgResult::empty() const {
        return size() == 0;
    }

    bool PgResult::is_ok() const {
        if (!res_) return false;
        const ExecStatusType status = PQresultStatus(res_);
        return (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK);
    }

    std::string PgResult::error_message() const {
        if (!res_) return "No result available";
        return PQresultErrorMessage(res_);
    }

    int PgResult::affected_rows() const {
        if (!res_) return 0;
        const char* val = PQcmdTuples(res_);
        return (val[0] == '\0') ? 0 : std::stoi(val);
    }

    PgRow PgResult::operator[](const size_t row_idx) const {
        return {res_, static_cast<int>(row_idx)};
    }

    PgRowIterator PgResult::begin() const {
        return {res_, 0};
    }

    PgRowIterator PgResult::end() const {
        return {res_, static_cast<int>(size())};
    }

} // namespace blaze
