#pragma once
#include <string>
#include <variant>
#include <utility>
#include <stdexcept>
#include <iostream>

namespace carbon {

enum class ErrorCode : uint32_t {
    ERR_SUCCESS = 0,
    ERR_STAT_SCAN_FAILED = 101,
    ERR_HASH_IO_ERROR = 102,
    ERR_CYCLE_DETECTED = 201,
    ERR_NODE_NOT_FOUND = 202,
    ERR_JVM_TOKEN_TIMEOUT = 301,
    ERR_SUBPROCESS_FAIL = 302,
    ERR_SUBPROCESS_TIMEOUT = 303,
    ERR_ATOMIC_RENAME_FAIL = 304,
    ERR_SQLITE_FAIL = 401,
    ERR_KEY_INVALID = 501,
    ERR_EROFS_BUILD_FAIL = 601,
    ERR_BROTLI_FAIL = 602,
    ERR_SUPER_OVERFLOW = 603,
    ERR_INVALID_ARGUMENT = 701,
    ERR_INTERNAL_ERROR = 999
};

struct CarbonError {
    ErrorCode code{ErrorCode::ERR_INTERNAL_ERROR};
    std::string message;

    CarbonError() = default;
    CarbonError(ErrorCode c, std::string msg) : code(c), message(std::move(msg)) {}

    std::string to_string() const {
        return "[" + std::to_string(static_cast<uint32_t>(code)) + "] " + message;
    }
};

template <typename T>
class CarbonResult {
public:
    CarbonResult(const T& val) : storage_(val) {}
    CarbonResult(T&& val) : storage_(std::move(val)) {}
    CarbonResult(const CarbonError& err) : storage_(err) {}
    CarbonResult(CarbonError&& err) : storage_(std::move(err)) {}

    bool is_ok() const noexcept {
        return std::holds_alternative<T>(storage_);
    }

    bool is_err() const noexcept {
        return std::holds_alternative<CarbonError>(storage_);
    }

    explicit operator bool() const noexcept {
        return is_ok();
    }

    T& value() & {
        if (is_err()) {
            throw std::runtime_error("Attempted to access value of failed CarbonResult: " + error().to_string());
        }
        return std::get<T>(storage_);
    }

    const T& value() const & {
        if (is_err()) {
            throw std::runtime_error("Attempted to access value of failed CarbonResult: " + error().to_string());
        }
        return std::get<T>(storage_);
    }

    T&& value() && {
        if (is_err()) {
            throw std::runtime_error("Attempted to access value of failed CarbonResult: " + error().to_string());
        }
        return std::get<T>(std::move(storage_));
    }

    T value_or(T&& default_val) const {
        return is_ok() ? std::get<T>(storage_) : std::forward<T>(default_val);
    }

    const CarbonError& error() const {
        if (is_ok()) {
            throw std::runtime_error("Attempted to access error of successful CarbonResult");
        }
        return std::get<CarbonError>(storage_);
    }

private:
    std::variant<T, CarbonError> storage_;
};

// Especialização para void
template <>
class CarbonResult<void> {
public:
    CarbonResult() : is_ok_(true), error_(ErrorCode::ERR_SUCCESS, "") {}
    CarbonResult(const CarbonError& err) : is_ok_(false), error_(err) {}
    CarbonResult(CarbonError&& err) : is_ok_(false), error_(std::move(err)) {}

    static CarbonResult<void> ok() {
        return CarbonResult<void>();
    }

    static CarbonResult<void> err(ErrorCode code, std::string msg) {
        return CarbonResult<void>(CarbonError(code, std::move(msg)));
    }

    bool is_ok() const noexcept { return is_ok_; }
    bool is_err() const noexcept { return !is_ok_; }
    explicit operator bool() const noexcept { return is_ok_; }

    const CarbonError& error() const {
        return error_;
    }

private:
    bool is_ok_{true};
    CarbonError error_;
};

} // namespace carbon
