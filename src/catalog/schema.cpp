#include "catalog/schema.hpp"

namespace duradb {

const ColumnSchema *TableSchema::find_column(std::string_view name) const {
    for (const ColumnSchema &column : columns) {
        if (column.name == name) {
            return &column;
        }
    }

    return nullptr;
}

std::optional<std::size_t> TableSchema::column_index(std::string_view name) const {
    if (const ColumnSchema *column = find_column(name)) {
        return column->ordinal;
    }

    return std::nullopt;
}

TableSchema table_schema_from_ast(const CreateTableStatement &statement) {
    TableSchema schema;
    schema.name = std::string(statement.table.table);

    for (std::size_t index = 0; index < statement.columns.size(); ++index) {
        schema.columns.push_back(ColumnSchema{std::string(statement.columns[index].name),
                                              statement.columns[index].type, index});
    }

    return schema;
}

} // namespace duradb
