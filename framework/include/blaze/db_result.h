#ifndef BLAZE_DB_RESULT_H
#define BLAZE_DB_RESULT_H

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <blaze/model.h>

namespace blaze {

class ResultImpl;

class Cell {
public:
    Cell(std::string_view v, bool is_null) : val_(v), null_(is_null) {}

    template<typename T>
    T as() const {
        if (null_) return T{};
        if constexpr (std::is_same_v<T, std::string>) {
            return std::string(val_);
        } else if constexpr (std::is_integral_v<T>) {
            if (val_.empty()) return T{};
            return boost::lexical_cast<T>(val_);
        } else if constexpr (std::is_floating_point_v<T>) {
            if (val_.empty()) return T{};
            return boost::lexical_cast<T>(val_);
        }
        return T{};
    }

private:
    std::string_view val_;
    bool null_;
};

// Abstract Interface for the Implementation (Internal Use Only)
class RowImpl {
public:
    virtual ~RowImpl() = default;
    virtual std::string_view get_column(size_t index) const = 0;
    virtual std::string_view get_column(std::string_view name) const = 0;
    virtual bool is_null(size_t index) const = 0;
    virtual bool is_null(std::string_view name) const = 0;
};

// Value Wrapper for a Row
class Row {
public:
    Row(std::shared_ptr<RowImpl> impl) : impl_(std::move(impl)) {}

    Cell operator[](size_t index) const {
        return Cell(impl_->get_column(index), impl_->is_null(index));
    }

    Cell operator[](std::string_view name) const {
        return Cell(impl_->get_column(name), impl_->is_null(name));
    }

    // Mini ORM Hook
    template<typename T>
    T as() const {
        return row_to_struct<T>(*this);
    }

private:
    std::shared_ptr<RowImpl> impl_;
};

// Abstract Interface for Result Implementation (Internal Use Only)
class ResultImpl {
public:
    virtual ~ResultImpl() = default;
    virtual size_t size() const = 0;
    virtual std::shared_ptr<RowImpl> get_row(size_t index) const = 0;
    virtual bool is_ok() const = 0;
    virtual std::string error_message() const = 0;
};

// Value Wrapper for the Result Set
class DbResult {
public:
    DbResult() = default;
    DbResult(std::shared_ptr<ResultImpl> impl) : impl_(std::move(impl)) {}

    size_t size() const { return impl_ ? impl_->size() : 0; }
    bool empty() const { return size() == 0; }
    
    // Returns Row by Value
    Row operator[](size_t index) const {
        if (!impl_) throw std::runtime_error("Empty DbResult");
        return Row(impl_->get_row(index));
    }
    
    bool is_ok() const { return impl_ ? impl_->is_ok() : false; }
    std::string error_message() const { return impl_ ? impl_->error_message() : "Empty Result"; }

private:
    std::shared_ptr<ResultImpl> impl_;
};

} // namespace blaze

#endif