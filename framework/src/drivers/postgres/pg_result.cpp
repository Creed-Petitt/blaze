#include <blaze/pg_result.h>
#include <libpq-fe.h>
#include <stdexcept>

namespace blaze {

    PgRow::PgRow(PGresult* res, const int row_idx)
        : res_(res), row_idx_(row_idx) {}

    std::string_view PgRow::get_column(size_t index) const {
        if (index >= static_cast<size_t>(PQnfields(res_))) {
            throw std::out_of_range("Column index out of bounds");
        }
        const char* val = PQgetvalue(res_, row_idx_, static_cast<int>(index));
        const int len = PQgetlength(res_, row_idx_, static_cast<int>(index));
        return std::string_view(val, len);
    }

    std::string_view PgRow::get_column(std::string_view name) const {
        const int col_idx = PQfnumber(res_, std::string(name).c_str());
        if (col_idx == -1) {
            throw std::runtime_error("Column not found: " + std::string(name));
        }
        return get_column(col_idx);
    }

    bool PgRow::is_null(size_t index) const {
        return PQgetisnull(res_, row_idx_, static_cast<int>(index));
    }

    bool PgRow::is_null(std::string_view name) const {
        const int col_idx = PQfnumber(res_, std::string(name).c_str());
        if (col_idx == -1) return true;
        return is_null(col_idx);
    }

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

    bool PgResult::is_ok() const {
        if (!res_) return false;
        const ExecStatusType status = PQresultStatus(res_);
        return (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK);
    }

    std::string PgResult::error_message() const {
        if (!res_) return "No result available";
        return PQresultErrorMessage(res_);
    }

    int64_t PgResult::affected_rows() const {
        if (!res_) return 0;
        const char* val = PQcmdTuples(res_);
        return (val[0] == '\0') ? 0 : std::stoll(val);
    }

        std::shared_ptr<RowImpl> PgResult::get_row(const size_t row_idx) const {
        return std::make_shared<PgRow>(res_, static_cast<int>(row_idx));
    }
} // namespace blaze
