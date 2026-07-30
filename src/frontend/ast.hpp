#pragma once

#include "catalog/catalog_constants.hpp"
#include "catalog/table_reference.hpp"

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
    CreateSchema,
    CreateDatabase,
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
    TableReference table;
    std::unique_ptr<Expression> where;
};

struct CreateTableStatement {
    TableReference table;
    std::vector<ColumnDefinition> columns;
};

struct CreateSchemaStatement {
    std::string_view schema;
};

struct CreateDatabaseStatement {
    std::string_view database;
};

struct InsertStatement {
    TableReference table;
    std::vector<std::unique_ptr<Expression>> values;
};

struct Statement {
    StatementKind kind;
    SelectStatement select;
    CreateTableStatement create_table;
    CreateSchemaStatement create_schema;
    CreateDatabaseStatement create_database;
    InsertStatement insert;
};

} // namespace duradb
