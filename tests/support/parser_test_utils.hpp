#pragma once

#include "frontend/ast.hpp"
#include "frontend/parser.hpp"

#include <gtest/gtest.h>

#include <string_view>

namespace duradb::test {

inline ParseResult<Statement> parse_statement(std::string_view sql) {
    Parser parser(sql);
    return parser.parse_statement();
}

inline void expect_parse_success(std::string_view sql) {
    const ParseResult<Statement> result = parse_statement(sql);
    ASSERT_TRUE(result.has_value()) << result.error().message;
}

inline void expect_parse_failure(std::string_view sql) {
    const ParseResult<Statement> result = parse_statement(sql);
    ASSERT_FALSE(result.has_value());
}

} // namespace duradb::test
