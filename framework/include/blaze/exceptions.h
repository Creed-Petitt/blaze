#ifndef BLAZE_EXCEPTIONS_H
#define BLAZE_EXCEPTIONS_H

#include <stdexcept>
#include <string>

namespace blaze {

/**
 * @brief Base class for all HTTP-related exceptions in Blaze.
 */
class HttpError : public std::runtime_error {
    int status_code_;
public:
    HttpError(int status, const std::string& msg) 
        : std::runtime_error(msg), status_code_(status) {}
    
    int status() const { return status_code_; }
};

/** @brief 400 Bad Request */
class BadRequest : public HttpError {
public:
    BadRequest(const std::string& msg = "Bad Request") : HttpError(400, msg) {}
};

/** @brief 401 Unauthorized */
class Unauthorized : public HttpError {
public:
    Unauthorized(const std::string& msg = "Unauthorized") : HttpError(401, msg) {}
};

/** @brief 403 Forbidden */
class Forbidden : public HttpError {
public:
    Forbidden(const std::string& msg = "Forbidden") : HttpError(403, msg) {}
};

/** @brief 404 Not Found */
class NotFound : public HttpError {
public:
    NotFound(const std::string& msg = "Not Found") : HttpError(404, msg) {}
};

/** @brief 500 Internal Server Error */
class InternalServerError : public HttpError {
public:
    InternalServerError(const std::string& msg = "Internal Server Error") : HttpError(500, msg) {}
};

} // namespace blaze

#endif
