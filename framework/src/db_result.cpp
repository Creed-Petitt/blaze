#include <blaze/db_result.h>
#include <blaze/exceptions.h>
#include <stdexcept>
#include <string>

namespace blaze {

// Row implementations
Cell Row::operator[](size_t index) const {
    return Cell(impl_->get_column(index), impl_->is_null(index));
}

Cell Row::operator[](std::string_view name) const {
    return Cell(impl_->get_column(name), impl_->is_null(name));
}

// DbResult implementations
size_t DbResult::size() const { 
    return impl_ ? impl_->size() : 0; 
}

bool DbResult::empty() const { 
    return size() == 0; 
}

int64_t DbResult::affected_rows() const { 
    return impl_ ? impl_->affected_rows() : 0; 
}

Row DbResult::operator[](size_t index) const {
    if (!impl_) throw InternalServerError("Database result access on empty result");
    if (index >= size()) throw InternalServerError("Database row index out of bounds: " + std::to_string(index));
    return Row(impl_->get_row(index));
}

bool DbResult::is_ok() const { 
    return impl_ ? impl_->is_ok() : false; 
}

std::string DbResult::error_message() const { 
    return impl_ ? impl_->error_message() : "Empty Result"; 
}

} // namespace blaze
