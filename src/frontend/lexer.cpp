#include "frontend/lexer.hpp"

#include <cctype>

namespace duradb {

namespace {

struct Keyword {
    std::string_view spelling;
    TokenKind kind;
};

constexpr Keyword kKeywords[] = {
    {"select", TokenKind::Select}, {"from", TokenKind::From},     {"where", TokenKind::Where},
    {"insert", TokenKind::Insert}, {"into", TokenKind::Into},     {"create", TokenKind::Create},
    {"table", TokenKind::Table},   {"values", TokenKind::Values}, {"int", TokenKind::Int},
    {"text", TokenKind::Text},     {"and", TokenKind::And},       {"or", TokenKind::Or},
    {"not", TokenKind::Not},       {"null", TokenKind::Null},     {"true", TokenKind::True},
    {"false", TokenKind::False},
};

bool equals_ignore_case(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) {
        return false;
    }

    for (std::size_t index = 0; index < left.size(); ++index) {
        const auto left_char = static_cast<unsigned char>(left[index]);
        const auto right_char = static_cast<unsigned char>(right[index]);
        if (std::tolower(left_char) != std::tolower(right_char)) {
            return false;
        }
    }

    return true;
}

} // namespace

const char *token_kind_name(TokenKind kind) {
    switch (kind) {
    case TokenKind::Select:
        return "Select";
    case TokenKind::From:
        return "From";
    case TokenKind::Where:
        return "Where";
    case TokenKind::Insert:
        return "Insert";
    case TokenKind::Into:
        return "Into";
    case TokenKind::Create:
        return "Create";
    case TokenKind::Table:
        return "Table";
    case TokenKind::Values:
        return "Values";
    case TokenKind::Int:
        return "Int";
    case TokenKind::Text:
        return "Text";
    case TokenKind::And:
        return "And";
    case TokenKind::Or:
        return "Or";
    case TokenKind::Not:
        return "Not";
    case TokenKind::Null:
        return "Null";
    case TokenKind::True:
        return "True";
    case TokenKind::False:
        return "False";
    case TokenKind::Identifier:
        return "Identifier";
    case TokenKind::IntegerLiteral:
        return "IntegerLiteral";
    case TokenKind::StringLiteral:
        return "StringLiteral";
    case TokenKind::Comma:
        return "Comma";
    case TokenKind::Semicolon:
        return "Semicolon";
    case TokenKind::LParen:
        return "LParen";
    case TokenKind::RParen:
        return "RParen";
    case TokenKind::Star:
        return "Star";
    case TokenKind::Equal:
        return "Equal";
    case TokenKind::NotEqual:
        return "NotEqual";
    case TokenKind::Less:
        return "Less";
    case TokenKind::LessEqual:
        return "LessEqual";
    case TokenKind::Greater:
        return "Greater";
    case TokenKind::GreaterEqual:
        return "GreaterEqual";
    case TokenKind::EndOfFile:
        return "EndOfFile";
    case TokenKind::Invalid:
        return "Invalid";
    }

    return "Unknown";
}

Lexer::Lexer(std::string_view input) : input_(input) {}

bool Lexer::at_end() const {
    return pos_ >= input_.size();
}

char Lexer::current_char() const {
    return peek();
}

char Lexer::peek() const {
    if (at_end()) {
        return '\0';
    }

    return input_[pos_];
}

char Lexer::advance() {
    if (at_end()) {
        return '\0';
    }

    const char current = input_[pos_++];
    if (current == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }

    return current;
}

bool Lexer::match(char expected) {
    if (peek() != expected) {
        return false;
    }

    advance();
    return true;
}

SourceLocation Lexer::current_location() const {
    return SourceLocation{pos_, line_, column_};
}

Token Lexer::make_token(TokenKind kind, SourceLocation start) const {
    return Token{kind, input_.substr(start.offset, pos_ - start.offset), start.line, start.column};
}

bool Lexer::is_whitespace(char character) {
    return character == ' ' || character == '\t' || character == '\n' || character == '\r';
}

bool Lexer::is_identifier_start(char character) {
    return std::isalpha(static_cast<unsigned char>(character)) || character == '_';
}

bool Lexer::is_identifier_continue(char character) {
    return std::isalnum(static_cast<unsigned char>(character)) || character == '_';
}

void Lexer::skip_line_comment() {
    while (!at_end() && peek() != '\n') {
        advance();
    }
}

void Lexer::skip_whitespace_and_comments() {
    while (!at_end()) {
        if (is_whitespace(peek())) {
            advance();
            continue;
        }

        if (peek() == '-' && pos_ + 1 < input_.size() && input_[pos_ + 1] == '-') {
            skip_line_comment();
            continue;
        }

        break;
    }
}

std::optional<TokenKind> Lexer::classify_keyword(std::string_view text) {
    for (const Keyword &keyword : kKeywords) {
        if (equals_ignore_case(text, keyword.spelling)) {
            return keyword.kind;
        }
    }

    return std::nullopt;
}

Token Lexer::scan_identifier_or_keyword(SourceLocation start) {
    while (is_identifier_continue(peek())) {
        advance();
    }

    const std::string_view text = input_.substr(start.offset, pos_ - start.offset);
    if (const std::optional<TokenKind> keyword = classify_keyword(text)) {
        return Token{*keyword, text, start.line, start.column};
    }

    return Token{TokenKind::Identifier, text, start.line, start.column};
}

Token Lexer::scan_number(SourceLocation start) {
    while (std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
    }

    return make_token(TokenKind::IntegerLiteral, start);
}

Token Lexer::scan_string(SourceLocation start) {
    advance();

    while (!at_end()) {
        const char character = advance();
        if (character != '\'') {
            continue;
        }

        if (peek() == '\'') {
            advance();
            continue;
        }

        return make_token(TokenKind::StringLiteral, start);
    }

    return Token{TokenKind::Invalid, input_.substr(start.offset), start.line, start.column};
}

Token Lexer::scan_punctuation(char punctuation, SourceLocation start) {
    switch (punctuation) {
    case ',':
        return make_token(TokenKind::Comma, start);
    case ';':
        return make_token(TokenKind::Semicolon, start);
    case '(':
        return make_token(TokenKind::LParen, start);
    case ')':
        return make_token(TokenKind::RParen, start);
    case '*':
        return make_token(TokenKind::Star, start);
    case '=':
        return make_token(TokenKind::Equal, start);
    case '<':
        if (match('=')) {
            return make_token(TokenKind::LessEqual, start);
        }
        if (match('>')) {
            return make_token(TokenKind::NotEqual, start);
        }
        return make_token(TokenKind::Less, start);
    case '>':
        if (match('=')) {
            return make_token(TokenKind::GreaterEqual, start);
        }
        return make_token(TokenKind::Greater, start);
    case '!':
        if (match('=')) {
            return make_token(TokenKind::NotEqual, start);
        }
        return make_token(TokenKind::Invalid, start);
    default:
        return make_token(TokenKind::Invalid, start);
    }
}

Token Lexer::next() {
    skip_whitespace_and_comments();

    const SourceLocation start = current_location();
    if (at_end()) {
        return Token{TokenKind::EndOfFile, {}, start.line, start.column};
    }

    const char character = current_char();
    if (is_identifier_start(character)) {
        return scan_identifier_or_keyword(start);
    }

    if (std::isdigit(static_cast<unsigned char>(character))) {
        return scan_number(start);
    }

    if (character == '\'') {
        return scan_string(start);
    }

    advance();
    return scan_punctuation(character, start);
}

} // namespace duradb
