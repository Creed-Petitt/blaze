#ifndef BLAZE_MYSQL_RESULT_H
#define BLAZE_MYSQL_RESULT_H

#include <mysql.h>
#include <string>
#include <vector>
#include <blaze/db_result.h>

namespace blaze {

    class MySqlResult;

    class MySqlRow : public RowImpl {
    public:
        MySqlRow(MYSQL_RES* res, MYSQL_ROW row, unsigned long* lengths);
        
        // Implementation
        std::string_view get_column(size_t index) const override;
        std::string_view get_column(std::string_view name) const override;
        bool is_null(size_t index) const override;
        bool is_null(std::string_view name) const override;

    private:
        MYSQL_RES* res_;
        MYSQL_ROW row_;
        unsigned long* lengths_;
    };

    class MySqlResult : public ResultImpl {
    public:
        explicit MySqlResult(MYSQL* conn, MYSQL_RES* res);
        ~MySqlResult();

        MySqlResult(MySqlResult&& other) noexcept;
        MySqlResult(const MySqlResult&) = delete;

        MySqlResult& operator=(MySqlResult&& other) noexcept;
        MySqlResult& operator=(const MySqlResult&) = delete;

        size_t size() const override;
        
        // Factory
        std::shared_ptr<RowImpl> get_row(size_t row_idx) const override;

        [[nodiscard]] MYSQL_RES* get_raw() const { return res_; }
        [[nodiscard]] MYSQL_ROW get_row_raw(size_t idx) const { return rows_[idx]; }
        [[nodiscard]] unsigned long* get_lengths_raw(size_t idx) const { return lengths_[idx]; }

        bool is_ok() const override { return ok_; }
        std::string error_message() const override { return error_; }
        
        // MySQL returns 64-bit counts (my_ulonglong). We cast to int for API consistency.
        int affected_rows() const override { return static_cast<int>(affected_rows_); }
        void set_affected_rows(uint64_t rows) { affected_rows_ = rows; }

    private:
        MYSQL* conn_ = nullptr;
        MYSQL_RES* res_ = nullptr;
        std::vector<MYSQL_ROW> rows_;
        std::vector<unsigned long*> lengths_;
        bool ok_ = true;
        std::string error_;
        uint64_t affected_rows_ = 0;
    };

} // namespace blaze

#endif
