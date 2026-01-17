#ifndef BLAZE_MYSQL_RESULT_H
#define BLAZE_MYSQL_RESULT_H

#include <mariadb/mysql.h>
#include <string>
#include <vector>

namespace blaze {

    class MySqlResult;

    class MySqlField {
    public:
        MySqlField(const char* data, unsigned long len);
        [[nodiscard]] bool is_null() const;

        template<typename T>
        [[nodiscard]] T as() const;

    private:
        const char* data_;
        unsigned long len_;
    };

    class MySqlRow {
    public:
        MySqlRow(MYSQL_RES* res, MYSQL_ROW row, unsigned long* lengths);
        MySqlField operator[](int col_idx) const;
        MySqlField operator[](const char* col_name) const;

    private:
        MYSQL_RES* res_;
        MYSQL_ROW row_;
        unsigned long* lengths_;
    };

    class MySqlRowIterator {
    public:
        MySqlRowIterator(MySqlResult* parent, int row_idx);
        [[nodiscard]] MySqlRow operator*() const;
        MySqlRowIterator& operator++();
        [[nodiscard]] bool operator!=(const MySqlRowIterator& other) const;

    private:
        MySqlResult* parent_;
        int row_idx_;
    };

    class MySqlResult {
    public:
        explicit MySqlResult(MYSQL* conn, MYSQL_RES* res);
        ~MySqlResult();

        MySqlResult(MySqlResult&& other) noexcept;
        MySqlResult(const MySqlResult&) = delete;

        MySqlResult& operator=(MySqlResult&& other) noexcept;
        MySqlResult& operator=(const MySqlResult&) = delete;

        size_t size() const;
        bool empty() const;
        MySqlRow operator[](size_t row_idx) const;

        MySqlRowIterator begin();
        MySqlRowIterator end();

        [[nodiscard]] bool is_ok() const { return ok_; }
        [[nodiscard]] std::string error_message() const { return error_; }

    private:
        MYSQL* conn_ = nullptr;
        MYSQL_RES* res_ = nullptr;
        std::vector<MYSQL_ROW> rows_;
        std::vector<unsigned long*> lengths_;
        bool ok_ = true;
        std::string error_;
    };

    template<> std::string MySqlField::as<std::string>() const;
    template<> int MySqlField::as<int>() const;

} // namespace blaze

#endif
