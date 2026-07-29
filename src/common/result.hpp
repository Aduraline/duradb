#pragma once

#include <optional>
#include <string>
#include <utility>

namespace duradb {

struct Error {
    std::string message;
};

template <typename T> class Result {
  public:
    static Result ok(T value) {
        Result result;
        result.value_ = std::move(value);
        return result;
    }

    static Result fail(Error error) {
        Result result;
        result.error_ = std::move(error);
        return result;
    }

    bool has_value() const {
        return value_.has_value();
    }

    const T &value() const {
        return value_.value();
    }

    T &value() {
        return value_.value();
    }

    const Error &error() const {
        return error_.value();
    }

  private:
    std::optional<T> value_;
    std::optional<Error> error_;
};

struct Unit {};

using Status = Result<Unit>;

} // namespace duradb
