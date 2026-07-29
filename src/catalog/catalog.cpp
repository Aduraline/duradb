#include "catalog/catalog.hpp"

namespace duradb {

namespace {

bool has_duplicate_columns(const TableSchema &schema) {
    for (std::size_t left = 0; left < schema.columns.size(); ++left) {
        for (std::size_t right = left + 1; right < schema.columns.size(); ++right) {
            if (schema.columns[left].name == schema.columns[right].name) {
                return true;
            }
        }
    }

    return false;
}

bool types_match(LogicalType expected, LogicalType actual) {
    return expected == actual;
}

} // namespace

Status Catalog::create_table(TableSchema schema) {
    if (schema.name.empty()) {
        return Status::fail(Error{"table name must not be empty"});
    }

    if (schema.columns.empty()) {
        return Status::fail(Error{"table must have at least one column"});
    }

    if (has_duplicate_columns(schema)) {
        return Status::fail(Error{"duplicate column name"});
    }

    if (tables_.contains(schema.name)) {
        return Status::fail(Error{"table already exists"});
    }

    tables_.emplace(schema.name, std::move(schema));
    return Status::ok(Unit{});
}

const TableSchema *Catalog::find_table(std::string_view name) const {
    const auto iterator = tables_.find(std::string(name));
    if (iterator == tables_.end()) {
        return nullptr;
    }

    return &iterator->second;
}

bool Catalog::table_exists(std::string_view name) const {
    return find_table(name) != nullptr;
}

Status Catalog::validate_insert(std::string_view table_name,
                                const std::vector<Value> &values) const {
    const TableSchema *table = find_table(table_name);
    if (table == nullptr) {
        return Status::fail(Error{"table not found"});
    }

    if (values.size() != table->columns.size()) {
        return Status::fail(Error{"insert column count mismatch"});
    }

    for (std::size_t index = 0; index < values.size(); ++index) {
        if (!types_match(table->columns[index].type, values[index].type)) {
            return Status::fail(Error{"insert type mismatch"});
        }
    }

    return Status::ok(Unit{});
}

} // namespace duradb
