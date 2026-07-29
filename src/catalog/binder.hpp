#pragma once

#include "catalog/catalog.hpp"
#include "catalog/schema.hpp"
#include "catalog/value.hpp"
#include "common/result.hpp"
#include "frontend/ast.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace duradb {

struct BoundCreateTableStatement {
    TableSchema schema;
};

struct BoundInsertStatement {
    const TableSchema *table;
    std::vector<Value> values;
};

struct BoundSelectStatement {
    const TableSchema *table;
    bool select_all;
    std::vector<std::size_t> column_ordinals;
    std::unique_ptr<Expression> where; // TODO: bind to typed expression tree
};

struct BoundStatement {
    enum class Kind { CreateTable, Insert, Select } kind;

    BoundCreateTableStatement create_table;
    BoundInsertStatement insert;
    BoundSelectStatement select;
};

class Binder {
  public:
    explicit Binder(const Catalog &catalog);

    Result<BoundStatement> bind(Statement statement) const;

  private:
    const Catalog &catalog_;

    Result<BoundStatement> bind_create_table(const CreateTableStatement &statement) const;
    Result<BoundStatement> bind_insert(const InsertStatement &statement) const;
    Result<BoundStatement> bind_select(SelectStatement statement) const;
};

} // namespace duradb
