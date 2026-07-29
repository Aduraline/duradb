#include "storage/heap_table.hpp"

namespace duradb {

Status HeapTable::create_table(TableSchema schema) {
    if (schema.name.empty()) {
        return Status::fail(Error{"table name must not be empty"});
    }

    if (tables_.contains(schema.name)) {
        return Status::fail(Error{"table already exists"});
    }

    const std::string table_name = schema.name;
    tables_.emplace(table_name, TableStorage{std::move(schema), {}});
    return Status::ok(Unit{});
}

Status HeapTable::insert(std::string_view table_name, Row row) {
    const auto iterator = tables_.find(std::string(table_name));
    if (iterator == tables_.end()) {
        return Status::fail(Error{"table not found"});
    }

    if (const Status validation = validate_row(row, iterator->second.schema);
        !validation.has_value()) {
        return validation;
    }

    iterator->second.rows.push_back(std::move(row));
    return Status::ok(Unit{});
}

Result<std::vector<Row>> HeapTable::scan(std::string_view table_name) const {
    const auto iterator = tables_.find(std::string(table_name));
    if (iterator == tables_.end()) {
        return Result<std::vector<Row>>::fail(Error{"table not found"});
    }

    return Result<std::vector<Row>>::ok(iterator->second.rows);
}

bool HeapTable::table_exists(std::string_view table_name) const {
    return tables_.contains(std::string(table_name));
}

} // namespace duradb
