#pragma once

#include "frontend/lexer.hpp"

#include <gtest/gtest.h>

#include <string_view>
#include <vector>

namespace duradb::test {

inline std::vector<TokenKind> collect_token_kinds(std::string_view sql) {
    Lexer lexer(sql);
    std::vector<TokenKind> kinds;

    while (true) {
        const Token token = lexer.next();
        kinds.push_back(token.kind);
        if (token.kind == TokenKind::EndOfFile) {
            break;
        }
    }

    return kinds;
}

inline Token first_token(std::string_view sql) {
    Lexer lexer(sql);
    return lexer.next();
}

inline void expect_token_stream(std::string_view sql, const std::vector<TokenKind> &expected) {
    EXPECT_EQ(collect_token_kinds(sql), expected);
}

} // namespace duradb::test
