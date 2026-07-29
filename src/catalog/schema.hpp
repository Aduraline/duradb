#pragma once

#include "frontend/ast.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace duradb {

struct ColumnSchema {
    std::string name;
    LogicalType type;
    std::size_t ordinal;
};

struct TableSchema {
    std::string name;
    std::vector<ColumnSchema> columns;

    const ColumnSchema *find_column(std::string_view name) const;
    std::optional<std::size_t> column_index(std::string_view name) const;
};

TableSchema table_schema_from_ast(const CreateTableStatement &statement);

} // namespace duradb
