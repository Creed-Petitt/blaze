#ifndef BLAZE_DB_RESULT_H
#define BLAZE_DB_RESULT_H

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <cstddef>
#include <cstdint>
#include <blaze/util/string.h>
#include <blaze/model.h>

namespace blaze {

class RowImpl;

/**
 * @brief Represents a single field value in a database row.
 */
class Cell {
public:
    Cell(std::string_view v, bool is_null) : val_(v), null_(is_null) {}

    /**
     * @brief Converts the cell value to the target type T.
     */
    template<typename T>
    T as() const {
        if (null_) return T{};
        return convert_string<T>(val_);
    }

private:
    std::string_view val_;
    bool null_;
};

class RowImpl {
public:
    virtual ~RowImpl() = default;
    virtual std::string_view get_column(size_t index) const = 0;
    virtual std::string_view get_column(std::string_view name) const = 0;
    virtual bool is_null(size_t index) const = 0;
    virtual bool is_null(std::string_view name) const = 0;
};

/**
 * @brief Value wrapper for a single row in a result set.
 */
class Row {
public:
    Row(std::shared_ptr<RowImpl> impl) : impl_(std::move(impl)) {}

    Cell operator[](size_t index) const;
    Cell operator[](std::string_view name) const;

    template<typename T>
    T as() const {
        return row_to_struct<T>(*this);
    }

private:
    std::shared_ptr<RowImpl> impl_;
};

class ResultImpl {
public:
    virtual ~ResultImpl() = default;
    virtual size_t size() const = 0;
    virtual std::shared_ptr<RowImpl> get_row(size_t index) const = 0;
    virtual bool is_ok() const = 0;
    virtual std::string error_message() const = 0;
    virtual int64_t affected_rows() const = 0;
};

/**
 * @brief Value wrapper for a complete database result set.
 */
class DbResult {
public:
    DbResult() = default;
    DbResult(std::shared_ptr<ResultImpl> impl) : impl_(std::move(impl)) {}

    size_t size() const;
    bool empty() const;
    int64_t affected_rows() const;
    
    Row operator[](size_t index) const;
    
    bool is_ok() const;
    std::string error_message() const;

private:
    std::shared_ptr<ResultImpl> impl_;
};

} // namespace blaze

#endif // BLAZE_DB_RESULT_H
