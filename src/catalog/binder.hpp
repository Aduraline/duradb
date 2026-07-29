#pragma once

#include "catalog/bound_expr.hpp"
#include "catalog/schema.hpp"
#include "catalog/value.hpp"
#include "common/result.hpp"
#include "engine/database_engine.hpp"
#include "frontend/ast.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace duradb {

struct BoundCreateTableStatement {
    TableSchema schema;
};

struct BoundInsertStatement {
    std::string table_name;
    std::vector<Value> values;
};

struct BoundSelectStatement {
    std::string table_name;
    bool select_all;
    std::vector<std::size_t> column_ordinals;
    std::unique_ptr<BoundExpression> where;
};

struct BoundStatement {
    enum class Kind { CreateTable, Insert, Select } kind;

    BoundCreateTableStatement create_table;
    BoundInsertStatement insert;
    BoundSelectStatement select;
};

class Binder {
  public:
    explicit Binder(const DatabaseEngine &engine);

    Result<BoundStatement> bind(Statement statement) const;

  private:
    const DatabaseEngine &engine_;

    Result<BoundStatement> bind_create_table(const CreateTableStatement &statement) const;
    Result<BoundStatement> bind_insert(const InsertStatement &statement) const;
    Result<BoundStatement> bind_select(SelectStatement statement) const;
};

} // namespace duradb
