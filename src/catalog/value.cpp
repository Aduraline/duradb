#include "catalog/value.hpp"

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

} // namespace

Value Value::from_int(std::int64_t value) {
    return Value{LogicalType::Int, value};
}

Value Value::from_text(std::string value) {
    return Value{LogicalType::Text, std::move(value)};
}

Value Value::from_expression(const Expression &expression) {
    if (const auto *integer = dynamic_cast<const IntegerLiteralExpression *>(&expression)) {
        return from_int(integer->value);
    }

    if (const auto *string = dynamic_cast<const StringLiteralExpression *>(&expression)) {
        return from_text(decode_string_literal(string->lexeme));
    }

    return from_text("");
}

std::int64_t Value::as_int() const {
    return std::get<std::int64_t>(payload);
}

const std::string &Value::as_text() const {
    return std::get<std::string>(payload);
}

} // namespace duradb
