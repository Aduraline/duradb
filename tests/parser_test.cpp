#include "support/parser_test_utils.hpp"

#include <gtest/gtest.h>

using duradb::BinaryExpression;
using duradb::BinaryOperator;
using duradb::ColumnRefExpression;
using duradb::IntegerLiteralExpression;
using duradb::LogicalType;
using duradb::StatementKind;
using duradb::StringLiteralExpression;
using duradb::test::expect_parse_failure;
using duradb::test::expect_parse_success;
using duradb::test::parse_statement;

namespace {

const BinaryExpression &as_binary_expression(const duradb::Expression &expression) {
    return dynamic_cast<const BinaryExpression &>(expression);
}

const ColumnRefExpression &as_column_ref(const duradb::Expression &expression) {
    return dynamic_cast<const ColumnRefExpression &>(expression);
}

const IntegerLiteralExpression &as_integer_literal(const duradb::Expression &expression) {
    return dynamic_cast<const IntegerLiteralExpression &>(expression);
}

const StringLiteralExpression &as_string_literal(const duradb::Expression &expression) {
    return dynamic_cast<const StringLiteralExpression &>(expression);
}

} // namespace

TEST(ParserTest, ParsesSelectWithWhereClause) {
    const auto result = parse_statement("SELECT name FROM users WHERE age > 18;");
    ASSERT_TRUE(result.has_value());

    const duradb::Statement &statement = result.value();
    ASSERT_EQ(statement.kind, StatementKind::Select);
    EXPECT_FALSE(statement.select.select_all);
    ASSERT_EQ(statement.select.columns.size(), 1U);
    EXPECT_EQ(as_column_ref(*statement.select.columns[0]).name, "name");
    EXPECT_EQ(statement.select.table, "users");
    ASSERT_NE(statement.select.where, nullptr);

    const BinaryExpression &predicate = as_binary_expression(*statement.select.where);
    EXPECT_EQ(predicate.op, BinaryOperator::Greater);
    EXPECT_EQ(as_column_ref(*predicate.left).name, "age");
    EXPECT_EQ(as_integer_literal(*predicate.right).value, 18);
}

TEST(ParserTest, ParsesSelectStar) {
    const auto result = parse_statement("SELECT * FROM users;");
    ASSERT_TRUE(result.has_value());

    const duradb::Statement &statement = result.value();
    ASSERT_EQ(statement.kind, StatementKind::Select);
    EXPECT_TRUE(statement.select.select_all);
    EXPECT_TRUE(statement.select.columns.empty());
    EXPECT_EQ(statement.select.table, "users");
    EXPECT_EQ(statement.select.where, nullptr);
}

TEST(ParserTest, ParsesCreateTableStatement) {
    const auto result = parse_statement("CREATE TABLE users (id INT, name TEXT);");
    ASSERT_TRUE(result.has_value());

    const duradb::Statement &statement = result.value();
    ASSERT_EQ(statement.kind, StatementKind::CreateTable);
    EXPECT_EQ(statement.create_table.table, "users");
    ASSERT_EQ(statement.create_table.columns.size(), 2U);
    EXPECT_EQ(statement.create_table.columns[0].name, "id");
    EXPECT_EQ(statement.create_table.columns[0].type, LogicalType::Int);
    EXPECT_EQ(statement.create_table.columns[1].name, "name");
    EXPECT_EQ(statement.create_table.columns[1].type, LogicalType::Text);
}

TEST(ParserTest, ParsesInsertStatement) {
    const auto result = parse_statement("INSERT INTO users VALUES (1, 'Alice');");
    ASSERT_TRUE(result.has_value());

    const duradb::Statement &statement = result.value();
    ASSERT_EQ(statement.kind, StatementKind::Insert);
    EXPECT_EQ(statement.insert.table, "users");
    ASSERT_EQ(statement.insert.values.size(), 2U);
    EXPECT_EQ(as_integer_literal(*statement.insert.values[0]).value, 1);
    EXPECT_EQ(as_string_literal(*statement.insert.values[1]).lexeme, "'Alice'");
}

TEST(ParserTest, ParsesAndExpressionWithPrecedence) {
    const auto result = parse_statement("SELECT * FROM users WHERE age > 18 AND name = 'Bob';");
    ASSERT_TRUE(result.has_value());

    const BinaryExpression &where = as_binary_expression(*result.value().select.where);
    EXPECT_EQ(where.op, BinaryOperator::And);

    const BinaryExpression &age_predicate = as_binary_expression(*where.left);
    EXPECT_EQ(age_predicate.op, BinaryOperator::Greater);

    const BinaryExpression &name_predicate = as_binary_expression(*where.right);
    EXPECT_EQ(name_predicate.op, BinaryOperator::Equal);
    EXPECT_EQ(as_string_literal(*name_predicate.right).lexeme, "'Bob'");
}

TEST(ParserTest, RejectsMissingSemicolon) {
    expect_parse_failure("SELECT name FROM users");
}

TEST(ParserTest, RejectsInvalidStatement) {
    expect_parse_failure("FROM users;");
}
