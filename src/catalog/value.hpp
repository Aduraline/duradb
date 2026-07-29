#pragma once

#include "frontend/ast.hpp"

#include <cstdint>
#include <string>

namespace duradb {

struct Value {
    LogicalType type;
    std::int64_t int_value{};
    std::string text_value;

    static Value from_int(std::int64_t value);
    static Value from_text(std::string value);

    static Value from_expression(const Expression &expression);
};

} // namespace duradb
