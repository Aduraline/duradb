#include "catalog/value.hpp"

#include <cassert>
#include <utility>

namespace duradb {

namespace {

std::string decode_string_literal(std::string_view lexeme) {
    if (lexeme.size() < 2 || lexeme.front() != '\'' || lexeme.back() != '\'') {
        return std::string(lexeme);
    }

    std::string decoded;
    decoded.reserve(lexeme.size());

    for (std::size_t index = 1; index + 1 < lexeme.size(); ++index) {
        const char character = lexeme[index];
        if (character == '\'' && index + 1 < lexeme.size() && lexeme[index + 1] == '\'') {
            decoded.push_back('\'');
            ++index;
            continue;
        }

        decoded.push_back(character);
    }

    return decoded;
}

bool payload_matches_type(LogicalType type,
                           const std::variant<std::int64_t, std::string> &payload) {
    switch (type) {
    case LogicalType::Int:
        return std::holds_alternative<std::int64_t>(payload);
    case LogicalType::Text:
        return std::holds_alternative<std::string>(payload);
    }

    return false;
}

} // namespace

Value Value::from_int(std::int64_t value) {
    return Value{LogicalType::Int, value};
}

Value Value::from_text(std::string value) {
    return Value{LogicalType::Text, std::move(value)};
}

Result<Value> Value::from_expression(const Expression &expression) {
    if (const auto *integer = dynamic_cast<const IntegerLiteralExpression *>(&expression)) {
        return Result<Value>::ok(from_int(integer->value));
    }

    if (const auto *string = dynamic_cast<const StringLiteralExpression *>(&expression)) {
        return Result<Value>::ok(from_text(decode_string_literal(string->lexeme)));
    }

    return Result<Value>::fail(Error{"expected literal expression"});
}

std::int64_t Value::as_int() const {
    assert(type == LogicalType::Int);
    assert(payload_matches_type(type, payload));
    return std::get<std::int64_t>(payload);
}

const std::string &Value::as_text() const {
    assert(type == LogicalType::Text);
    assert(payload_matches_type(type, payload));
    return std::get<std::string>(payload);
}

} // namespace duradb
