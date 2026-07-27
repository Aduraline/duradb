#pragma once

#include <cstddef>
#include <optional>
#include <string_view>

namespace duradb {

enum class TokenKind {
    Select,
    From,
    Where,
    Insert,
    Into,
    Create,
    Table,
    Values,
    Int,
    Text,
    And,
    Or,
    Not,
    Null,
    True,
    False,

    Identifier,
    IntegerLiteral,
    StringLiteral,

    Comma,
    Semicolon,
    LParen,
    RParen,
    Star,

    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    EndOfFile,
    Invalid,
};

struct SourceLocation {
    std::size_t offset{};
    std::size_t line{1};
    std::size_t column{1};
};

struct Token {
    TokenKind kind;
    std::string_view lexeme;
    std::size_t line;
    std::size_t column;
};

const char *token_kind_name(TokenKind kind);

class Lexer {
  public:
    explicit Lexer(std::string_view input);

    Token next();

  private:
    std::string_view input_;
    std::size_t pos_{0};
    std::size_t line_{1};
    std::size_t column_{1};

    bool at_end() const;
    char current_char() const;
    char peek() const;
    char advance();
    bool match(char expected);

    SourceLocation current_location() const;
    Token make_token(TokenKind kind, SourceLocation start) const;

    void skip_whitespace_and_comments();
    void skip_line_comment();

    Token scan_identifier_or_keyword(SourceLocation start);
    Token scan_number(SourceLocation start);
    Token scan_string(SourceLocation start);
    Token scan_punctuation(char punctuation, SourceLocation start);

    static std::optional<TokenKind> classify_keyword(std::string_view text);
    static bool is_identifier_start(char character);
    static bool is_identifier_continue(char character);
    static bool is_whitespace(char character);
};

} // namespace duradb
