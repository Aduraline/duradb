#pragma once

#include "frontend/ast.hpp"
#include "frontend/lexer.hpp"
#include "frontend/parse_result.hpp"

#include <memory>
#include <string_view>

namespace duradb {

class Parser {
  public:
    explicit Parser(std::string_view sql);

    ParseResult<Statement> parse_statement();

  private:
    Lexer lexer_;
    Token current_;

    void advance();
    bool check(TokenKind kind) const;
    bool match(TokenKind kind);

    ParseError make_error(std::string message) const;
    ParseResult<Statement> fail(std::string message) const;

    ParseResult<Statement> parse_select_statement();
    ParseResult<Statement> parse_create_table_statement();
    ParseResult<Statement> parse_insert_statement();

    ParseResult<std::unique_ptr<Expression>> parse_expression();
    ParseResult<std::unique_ptr<Expression>> parse_or_expression();
    ParseResult<std::unique_ptr<Expression>> parse_and_expression();
    ParseResult<std::unique_ptr<Expression>> parse_comparison_expression();
    ParseResult<std::unique_ptr<Expression>> parse_primary_expression();

    ParseResult<LogicalType> parse_column_type();
    ParseResult<BinaryOperator> parse_binary_operator();

    ParseResult<std::unique_ptr<Expression>> parse_column_reference();
    ParseResult<std::unique_ptr<Expression>> parse_integer_literal();
    ParseResult<std::unique_ptr<Expression>> parse_string_literal();

    bool is_comparison_operator(TokenKind kind) const;
};

} // namespace duradb
