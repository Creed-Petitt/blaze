#ifndef BLAZE_POSTGRES_RESULT_H
#define BLAZE_POSTGRES_RESULT_H

#include <libpq-fe.h>
#include <string>

namespace blaze {

    class PgResult;

    // PgField represents a view into a single cell
    class PgField {
    public:
        PgField(const char* data, int len);

        [[nodiscard]] bool is_null() const;

        // Conversion logic
        template<typename T>
        [[nodiscard]] T as() const;

    private:
        const char* data_;
        int len_;
    };


    // PgRow represents a view into a single row
    class PgRow {
    public:
        PgRow(PGresult* res, int row_idx);

        // Accessors
        PgField operator[](int col_idx) const;
        PgField operator[](const char* col_name) const;

    private:
        PGresult* res_;
        int row_idx_;
    };


    class PgRowIterator {
    public:
        PgRowIterator(PGresult* res, int row_idx);

        [[nodiscard]] PgRow operator*() const;
        PgRowIterator& operator++();
        [[nodiscard]] bool operator!=(const PgRowIterator& other) const;

    private:
        PGresult* res_;
        int row_idx_;
    };

    // PgResult represents the owner of the dataset
    class PgResult {
    public:
        explicit PgResult(PGresult* res);
        ~PgResult();

        PgResult(PgResult&& other) noexcept;
        PgResult(const PgResult&) = delete;

        PgResult& operator=(PgResult&& other) noexcept;
        PgResult& operator=(const PgResult&) = delete;

        // Data Access
        size_t size() const;
        bool empty() const;
        PgRow operator[](size_t row_idx) const;

        [[nodiscard]] PgRowIterator begin() const;
        [[nodiscard]] PgRowIterator end() const;


        // Metadata
        [[nodiscard]] int affected_rows() const;

        // Check if query succeeded
        [[nodiscard]] bool is_ok() const;
        [[nodiscard]] std::string error_message() const;

    private:
        PGresult* res_;
    };

    // Template specializations
    template<> std::string PgField::as<std::string>() const;
    template<> int PgField::as<int>() const;

} // namespace blaze

#endif // BLAZE_POSTGRES_RESULT_H
