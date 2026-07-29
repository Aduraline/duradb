#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace duradb {

enum class LogicalType {
    Int,
    Text,
};

enum class BinaryOperator {
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    And,
    Or,
};

enum class StatementKind {
    Select,
    CreateTable,
    Insert,
};

struct Expression {
    virtual ~Expression() = default;
};

struct ColumnRefExpression : Expression {
    std::string_view name;
};

struct IntegerLiteralExpression : Expression {
    std::int64_t value;
};

struct StringLiteralExpression : Expression {
    std::string_view lexeme;
};

struct BinaryExpression : Expression {
    BinaryOperator op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
};

struct ColumnDefinition {
    std::string_view name;
    LogicalType type;
};

struct SelectStatement {
    bool select_all{false};
    std::vector<std::unique_ptr<Expression>> columns;
    std::string_view table;
    std::unique_ptr<Expression> where;
};

struct CreateTableStatement {
    std::string_view table;
    std::vector<ColumnDefinition> columns;
};

struct InsertStatement {
    std::string_view table;
    std::vector<std::unique_ptr<Expression>> values;
};

struct Statement {
    StatementKind kind;
    SelectStatement select;
    CreateTableStatement create_table;
    InsertStatement insert;
};

} // namespace duradb
