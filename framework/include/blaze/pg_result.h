#ifndef BLAZE_POSTGRES_RESULT_H
#define BLAZE_POSTGRES_RESULT_H

#include <libpq-fe.h>
#include <string>
#include <blaze/db_result.h>

namespace blaze {

    class PgResult;

    // PgRow represents a view into a single row
    class PgRow : public RowImpl {
    public:
        PgRow(PGresult* res, int row_idx);

        // Implementation
        std::string_view get_column(size_t index) const override;
        std::string_view get_column(std::string_view name) const override;
        bool is_null(size_t index) const override;
        bool is_null(std::string_view name) const override;

    private:
        PGresult* res_;
        int row_idx_;
    };

    // PgResult represents the owner of the dataset
    class PgResult : public ResultImpl {
    public:
        explicit PgResult(PGresult* res);
        ~PgResult();

        PgResult(PgResult&& other) noexcept;
        PgResult(const PgResult&) = delete;

        PgResult& operator=(PgResult&& other) noexcept;
        PgResult& operator=(const PgResult&) = delete;

        // Data Access
        size_t size() const override;
        
        // Returns Row Implementation
        std::shared_ptr<RowImpl> get_row(size_t row_idx) const override;

        // Metadata
        [[nodiscard]] int64_t affected_rows() const override;

        // Check if query succeeded
        bool is_ok() const override;
        std::string error_message() const override;

        [[nodiscard]] PGresult* get_raw() const { return res_; }

    private:
        PGresult* res_;
    };

} // namespace blaze

#endif // BLAZE_POSTGRES_RESULT_H
