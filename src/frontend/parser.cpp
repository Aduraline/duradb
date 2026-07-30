#include "frontend/parser.hpp"

#include <charconv>
#include <utility>

namespace duradb {

namespace {

std::unique_ptr<BinaryExpression> make_binary_expression(BinaryOperator op,
                                                         std::unique_ptr<Expression> left,
                                                         std::unique_ptr<Expression> right) {
    auto expression = std::make_unique<BinaryExpression>();
    expression->op = op;
    expression->left = std::move(left);
    expression->right = std::move(right);
    return expression;
}

} // namespace

Parser::Parser(std::string_view sql) : lexer_(sql) {
    advance();
}

void Parser::advance() {
    current_ = lexer_.next();
}

bool Parser::check(TokenKind kind) const {
    return current_.kind == kind;
}

bool Parser::match(TokenKind kind) {
    if (!check(kind)) {
        return false;
    }

    advance();
    return true;
}

ParseError Parser::make_error(std::string message) const {
    return ParseError{std::move(message), current_.line, current_.column};
}

ParseResult<Statement> Parser::fail(std::string message) const {
    return ParseResult<Statement>::fail(make_error(std::move(message)));
}

bool Parser::is_comparison_operator(TokenKind kind) const {
    switch (kind) {
    case TokenKind::Equal:
    case TokenKind::NotEqual:
    case TokenKind::Less:
    case TokenKind::LessEqual:
    case TokenKind::Greater:
    case TokenKind::GreaterEqual:
        return true;
    default:
        return false;
    }
}

ParseResult<Statement> Parser::parse_statement() {
    if (check(TokenKind::Select)) {
        return parse_select_statement();
    }

    if (check(TokenKind::Create)) {
        return parse_create_statement();
    }

    if (check(TokenKind::Insert)) {
        return parse_insert_statement();
    }

    return fail("expected SELECT, CREATE, or INSERT statement");
}

bool Parser::match_statement_terminator() {
    if (match(TokenKind::Semicolon)) {
        return true;
    }

    return check(TokenKind::EndOfFile);
}

ParseResult<Statement> Parser::finish_statement(Statement statement) {
    if (!match_statement_terminator()) {
        return fail("expected ';'");
    }

    if (!check(TokenKind::EndOfFile)) {
        return fail("unexpected input after statement");
    }

    return ParseResult<Statement>::ok(std::move(statement));
}

ParseResult<Statement> Parser::parse_select_statement() {
    if (!match(TokenKind::Select)) {
        return fail("expected SELECT");
    }

    SelectStatement select;

    if (match(TokenKind::Star)) {
        select.select_all = true;
    } else {
        ParseResult<std::unique_ptr<Expression>> column = parse_column_reference();
        if (!column.has_value()) {
            return ParseResult<Statement>::fail(column.error());
        }

        select.columns.push_back(std::move(column.value()));

        while (match(TokenKind::Comma)) {
            column = parse_column_reference();
            if (!column.has_value()) {
                return ParseResult<Statement>::fail(column.error());
            }

            select.columns.push_back(std::move(column.value()));
        }
    }

    if (!match(TokenKind::From)) {
        return fail("expected FROM");
    }

    ParseResult<TableReference> table = parse_table_reference();
    if (!table.has_value()) {
        return ParseResult<Statement>::fail(table.error());
    }

    select.table = table.value();

    if (match(TokenKind::Where)) {
        ParseResult<std::unique_ptr<Expression>> where = parse_expression();
        if (!where.has_value()) {
            return ParseResult<Statement>::fail(where.error());
        }

        select.where = std::move(where.value());
    }

    Statement statement;
    statement.kind = StatementKind::Select;
    statement.select = std::move(select);
    return finish_statement(std::move(statement));
}

ParseResult<Statement> Parser::parse_create_statement() {
    if (!match(TokenKind::Create)) {
        return fail("expected CREATE");
    }

    if (check(TokenKind::Database)) {
        return parse_create_database_statement();
    }

    if (check(TokenKind::Schema)) {
        return parse_create_schema_statement();
    }

    if (check(TokenKind::Table)) {
        return parse_create_table_statement();
    }

    return fail("expected DATABASE, SCHEMA, or TABLE");
}

ParseResult<Statement> Parser::parse_create_database_statement() {
    if (!match(TokenKind::Database)) {
        return fail("expected DATABASE");
    }

    if (!check(TokenKind::Identifier)) {
        return fail("expected database name");
    }

    CreateDatabaseStatement create_database;
    create_database.database = current_.lexeme;
    advance();

    Statement statement;
    statement.kind = StatementKind::CreateDatabase;
    statement.create_database = create_database;
    return finish_statement(std::move(statement));
}

ParseResult<Statement> Parser::parse_create_schema_statement() {
    if (!match(TokenKind::Schema)) {
        return fail("expected SCHEMA");
    }

    if (!check(TokenKind::Identifier)) {
        return fail("expected schema name");
    }

    CreateSchemaStatement create_schema;
    create_schema.schema = current_.lexeme;
    advance();

    Statement statement;
    statement.kind = StatementKind::CreateSchema;
    statement.create_schema = create_schema;
    return finish_statement(std::move(statement));
}

ParseResult<Statement> Parser::parse_create_table_statement() {
    if (!match(TokenKind::Table)) {
        return fail("expected TABLE");
    }

    ParseResult<TableReference> table = parse_table_reference();
    if (!table.has_value()) {
        return ParseResult<Statement>::fail(table.error());
    }

    CreateTableStatement create_table;
    create_table.table = table.value();

    if (!match(TokenKind::LParen)) {
        return fail("expected '('");
    }

    if (!check(TokenKind::Identifier)) {
        return fail("expected column name");
    }

    ColumnDefinition column;
    column.name = current_.lexeme;
    advance();

    ParseResult<LogicalType> column_type = parse_column_type();
    if (!column_type.has_value()) {
        return ParseResult<Statement>::fail(column_type.error());
    }

    column.type = column_type.value();
    create_table.columns.push_back(column);

    while (match(TokenKind::Comma)) {
        if (!check(TokenKind::Identifier)) {
            return fail("expected column name");
        }

        column.name = current_.lexeme;
        advance();

        column_type = parse_column_type();
        if (!column_type.has_value()) {
            return ParseResult<Statement>::fail(column_type.error());
        }

        column.type = column_type.value();
        create_table.columns.push_back(column);
    }

    if (!match(TokenKind::RParen)) {
        return fail("expected ')'");
    }

    Statement statement;
    statement.kind = StatementKind::CreateTable;
    statement.create_table = std::move(create_table);
    return finish_statement(std::move(statement));
}

ParseResult<Statement> Parser::parse_insert_statement() {
    if (!match(TokenKind::Insert)) {
        return fail("expected INSERT");
    }

    if (!match(TokenKind::Into)) {
        return fail("expected INTO");
    }

    ParseResult<TableReference> table = parse_table_reference();
    if (!table.has_value()) {
        return ParseResult<Statement>::fail(table.error());
    }

    InsertStatement insert;
    insert.table = table.value();

    if (!match(TokenKind::Values)) {
        return fail("expected VALUES");
    }

    if (!match(TokenKind::LParen)) {
        return fail("expected '('");
    }

    ParseResult<std::unique_ptr<Expression>> value = parse_primary_expression();
    if (!value.has_value()) {
        return ParseResult<Statement>::fail(value.error());
    }

    insert.values.push_back(std::move(value.value()));

    while (match(TokenKind::Comma)) {
        value = parse_primary_expression();
        if (!value.has_value()) {
            return ParseResult<Statement>::fail(value.error());
        }

        insert.values.push_back(std::move(value.value()));
    }

    if (!match(TokenKind::RParen)) {
        return fail("expected ')'");
    }

    Statement statement;
    statement.kind = StatementKind::Insert;
    statement.insert = std::move(insert);
    return finish_statement(std::move(statement));
}

ParseResult<TableReference> Parser::parse_table_reference() {
    if (!check(TokenKind::Identifier)) {
        return ParseResult<TableReference>::fail(make_error("expected table name"));
    }

    TableReference reference;
    reference.schema = {};
    reference.table = current_.lexeme;
    advance();

    if (match(TokenKind::Dot)) {
        if (!check(TokenKind::Identifier)) {
            return ParseResult<TableReference>::fail(make_error("expected table name"));
        }

        reference.schema = reference.table;
        reference.table = current_.lexeme;
        advance();
    }

    return ParseResult<TableReference>::ok(reference);
}

ParseResult<LogicalType> Parser::parse_column_type() {
    if (match(TokenKind::Int)) {
        return ParseResult<LogicalType>::ok(LogicalType::Int);
    }

    if (match(TokenKind::Text)) {
        return ParseResult<LogicalType>::ok(LogicalType::Text);
    }

    return ParseResult<LogicalType>::fail(make_error("expected INT or TEXT column type"));
}

ParseResult<std::unique_ptr<Expression>> Parser::parse_expression() {
    return parse_or_expression();
}

ParseResult<std::unique_ptr<Expression>> Parser::parse_or_expression() {
    ParseResult<std::unique_ptr<Expression>> left = parse_and_expression();
    if (!left.has_value()) {
        return left;
    }

    while (match(TokenKind::Or)) {
        ParseResult<std::unique_ptr<Expression>> right = parse_and_expression();
        if (!right.has_value()) {
            return right;
        }

        left = ParseResult<std::unique_ptr<Expression>>::ok(make_binary_expression(
            BinaryOperator::Or, std::move(left.value()), std::move(right.value())));
    }

    return left;
}

ParseResult<std::unique_ptr<Expression>> Parser::parse_and_expression() {
    ParseResult<std::unique_ptr<Expression>> left = parse_comparison_expression();
    if (!left.has_value()) {
        return left;
    }

    while (match(TokenKind::And)) {
        ParseResult<std::unique_ptr<Expression>> right = parse_comparison_expression();
        if (!right.has_value()) {
            return right;
        }

        left = ParseResult<std::unique_ptr<Expression>>::ok(make_binary_expression(
            BinaryOperator::And, std::move(left.value()), std::move(right.value())));
    }

    return left;
}

ParseResult<std::unique_ptr<Expression>> Parser::parse_comparison_expression() {
    ParseResult<std::unique_ptr<Expression>> left = parse_primary_expression();
    if (!left.has_value()) {
        return left;
    }

    if (!is_comparison_operator(current_.kind)) {
        return left;
    }

    ParseResult<BinaryOperator> op = parse_binary_operator();
    if (!op.has_value()) {
        return ParseResult<std::unique_ptr<Expression>>::fail(op.error());
    }

    ParseResult<std::unique_ptr<Expression>> right = parse_primary_expression();
    if (!right.has_value()) {
        return right;
    }

    return ParseResult<std::unique_ptr<Expression>>::ok(
        make_binary_expression(op.value(), std::move(left.value()), std::move(right.value())));
}

ParseResult<std::unique_ptr<Expression>> Parser::parse_primary_expression() {
    if (check(TokenKind::Identifier)) {
        return parse_column_reference();
    }

    if (check(TokenKind::IntegerLiteral)) {
        return parse_integer_literal();
    }

    if (check(TokenKind::StringLiteral)) {
        return parse_string_literal();
    }

    if (match(TokenKind::LParen)) {
        ParseResult<std::unique_ptr<Expression>> expression = parse_expression();
        if (!expression.has_value()) {
            return expression;
        }

        if (!match(TokenKind::RParen)) {
            return ParseResult<std::unique_ptr<Expression>>::fail(make_error("expected ')'"));
        }

        return expression;
    }

    return ParseResult<std::unique_ptr<Expression>>::fail(make_error("expected expression"));
}

ParseResult<BinaryOperator> Parser::parse_binary_operator() {
    switch (current_.kind) {
    case TokenKind::Equal:
        advance();
        return ParseResult<BinaryOperator>::ok(BinaryOperator::Equal);
    case TokenKind::NotEqual:
        advance();
        return ParseResult<BinaryOperator>::ok(BinaryOperator::NotEqual);
    case TokenKind::Less:
        advance();
        return ParseResult<BinaryOperator>::ok(BinaryOperator::Less);
    case TokenKind::LessEqual:
        advance();
        return ParseResult<BinaryOperator>::ok(BinaryOperator::LessEqual);
    case TokenKind::Greater:
        advance();
        return ParseResult<BinaryOperator>::ok(BinaryOperator::Greater);
    case TokenKind::GreaterEqual:
        advance();
        return ParseResult<BinaryOperator>::ok(BinaryOperator::GreaterEqual);
    default:
        return ParseResult<BinaryOperator>::fail(make_error("expected comparison operator"));
    }
}

ParseResult<std::unique_ptr<Expression>> Parser::parse_column_reference() {
    if (!check(TokenKind::Identifier)) {
        return ParseResult<std::unique_ptr<Expression>>::fail(make_error("expected column name"));
    }

    auto expression = std::make_unique<ColumnRefExpression>();
    expression->name = current_.lexeme;
    advance();
    return ParseResult<std::unique_ptr<Expression>>::ok(std::move(expression));
}

ParseResult<std::unique_ptr<Expression>> Parser::parse_integer_literal() {
    if (!check(TokenKind::IntegerLiteral)) {
        return ParseResult<std::unique_ptr<Expression>>::fail(
            make_error("expected integer literal"));
    }

    std::int64_t value = 0;
    const std::string_view lexeme = current_.lexeme;
    const auto result = std::from_chars(lexeme.data(), lexeme.data() + lexeme.size(), value);
    if (result.ec != std::errc{}) {
        return ParseResult<std::unique_ptr<Expression>>::fail(
            make_error("invalid integer literal"));
    }

    auto expression = std::make_unique<IntegerLiteralExpression>();
    expression->value = value;
    advance();
    return ParseResult<std::unique_ptr<Expression>>::ok(std::move(expression));
}

ParseResult<std::unique_ptr<Expression>> Parser::parse_string_literal() {
    if (!check(TokenKind::StringLiteral)) {
        return ParseResult<std::unique_ptr<Expression>>::fail(
            make_error("expected string literal"));
    }

    auto expression = std::make_unique<StringLiteralExpression>();
    expression->lexeme = current_.lexeme;
    advance();
    return ParseResult<std::unique_ptr<Expression>>::ok(std::move(expression));
}

} // namespace duradb
