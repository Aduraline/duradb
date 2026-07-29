#include "engine/database_engine.hpp"

#include "common/string_view_hash.hpp"

#include <cstdlib>

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

#ifndef NDEBUG
void assert_row_matches_schema(const Row &row, const TableSchema &schema) {
    if (const Status validation = validate_row(row, schema); !validation.has_value()) {
        std::abort();
    }
}
#endif

} // namespace

DatabaseEngine::TableStorage *DatabaseEngine::find_storage(std::string_view table_name) {
    const auto iterator = tables_.find(table_name);
    if (iterator == tables_.end()) {
        return nullptr;
    }

    return &iterator->second;
}

const DatabaseEngine::TableStorage *
DatabaseEngine::find_storage(std::string_view table_name) const {
    const auto iterator = tables_.find(table_name);
    if (iterator == tables_.end()) {
        return nullptr;
    }

    return &iterator->second;
}

Status DatabaseEngine::create_table(TableSchema schema) {
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

    const std::string table_name = schema.name;
    tables_.emplace(table_name, TableStorage{std::move(schema), {}});
    return Status::ok(Unit{});
}

const TableSchema *DatabaseEngine::find_table(std::string_view name) const {
    const TableStorage *storage = find_storage(name);
    if (storage == nullptr) {
        return nullptr;
    }

    return &storage->schema;
}

bool DatabaseEngine::table_exists(std::string_view name) const {
    return find_storage(name) != nullptr;
}

Status DatabaseEngine::validate_insert(std::string_view table_name,
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

Status DatabaseEngine::insert(std::string_view table_name, Row row) {
    TableStorage *storage = find_storage(table_name);
    if (storage == nullptr) {
        return Status::fail(Error{"table not found"});
    }

#ifndef NDEBUG
    assert_row_matches_schema(row, storage->schema);
#endif

    storage->rows.push_back(std::move(row));
    return Status::ok(Unit{});
}

Status DatabaseEngine::insert_batch(std::string_view table_name, std::span<Row> rows) {
    TableStorage *storage = find_storage(table_name);
    if (storage == nullptr) {
        return Status::fail(Error{"table not found"});
    }

    if (rows.empty()) {
        return Status::ok(Unit{});
    }

#ifndef NDEBUG
    for (const Row &row : rows) {
        assert_row_matches_schema(row, storage->schema);
    }
#endif

    storage->rows.reserve(storage->rows.size() + rows.size());
    for (Row &row : rows) {
        storage->rows.push_back(std::move(row));
    }

    // TODO: columnar append buffer for write-heavy ingest
    return Status::ok(Unit{});
}

} // namespace duradb
