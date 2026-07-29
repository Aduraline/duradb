#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <utility>

namespace duradb {

struct ParseError {
    std::string message;
    std::size_t line;
    std::size_t column;
};

template <typename T> class ParseResult {
  public:
    static ParseResult ok(T value) {
        ParseResult result;
        result.value_ = std::move(value);
        return result;
    }

    static ParseResult fail(ParseError error) {
        ParseResult result;
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

    const ParseError &error() const {
        return error_.value();
    }

  private:
    std::optional<T> value_;
    std::optional<ParseError> error_;
};

} // namespace duradb
