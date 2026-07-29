#pragma once

#include "common/result.hpp"
#include "frontend/ast.hpp"

#include <cstdint>
#include <string>
#include <variant>

namespace duradb {

struct Value {
    LogicalType type;
    std::variant<std::int64_t, std::string> payload;

    static Value from_int(std::int64_t value);
    static Value from_text(std::string value);

    static Result<Value> from_expression(const Expression &expression);

    std::int64_t as_int() const;
    const std::string &as_text() const;
};

} // namespace duradb
