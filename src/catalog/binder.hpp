#pragma once

#include "catalog/bound_expr.hpp"
#include "catalog/schema.hpp"
#include "catalog/value.hpp"
#include "common/result.hpp"
#include "engine/session.hpp"
#include "frontend/ast.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace duradb {

struct BoundCreateTableStatement {
    std::string schema_name;
    TableSchema schema;
};

struct BoundCreateSchemaStatement {
    std::string schema_name;
};

struct BoundCreateDatabaseStatement {
    std::string database_name;
};

struct BoundInsertStatement {
    std::string schema_name;
    std::string table_name;
    std::vector<Value> values;
};

struct BoundSelectStatement {
    std::string schema_name;
    std::string table_name;
    bool select_all;
    std::vector<std::size_t> column_ordinals;
    std::unique_ptr<BoundExpression> where;
};

struct BoundStatement {
    enum class Kind { CreateTable, CreateSchema, CreateDatabase, Insert, Select } kind;

    BoundCreateTableStatement create_table;
    BoundCreateSchemaStatement create_schema;
    BoundCreateDatabaseStatement create_database;
    BoundInsertStatement insert;
    BoundSelectStatement select;
};

class Binder {
  public:
    explicit Binder(const Session &session);

    Result<BoundStatement> bind(Statement statement) const;

  private:
    const Session &session_;

    Result<BoundStatement> bind_create_table(const CreateTableStatement &statement) const;
    Result<BoundStatement> bind_create_schema(const CreateSchemaStatement &statement) const;
    Result<BoundStatement> bind_create_database(const CreateDatabaseStatement &statement) const;
    Result<BoundStatement> bind_insert(const InsertStatement &statement) const;
    Result<BoundStatement> bind_select(SelectStatement statement) const;
};

} // namespace duradb
