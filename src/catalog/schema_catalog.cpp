#include "catalog/schema_catalog.hpp"

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

#ifndef NDEBUG
void assert_row_matches_schema(const Row &row, const TableSchema &schema) {
    if (const Status validation = validate_row(row, schema); !validation.has_value()) {
        std::abort();
    }
}
#endif

} // namespace

SchemaCatalog::SchemaCatalog(std::string name) : name_(std::move(name)) {}

std::string_view SchemaCatalog::name() const { return name_; }

TableStorage *SchemaCatalog::find_storage(std::string_view table_name) {
    const auto iterator = tables_.find(table_name);
    if (iterator == tables_.end()) {
        return nullptr;
    }

    return &iterator->second;
}

const TableStorage *SchemaCatalog::find_storage(std::string_view table_name) const {
    const auto iterator = tables_.find(table_name);
    if (iterator == tables_.end()) {
        return nullptr;
    }

    return &iterator->second;
}

Status SchemaCatalog::create_table(TableSchema schema) {
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
    tables_.emplace(table_name, TableStorage{std::move(schema)});
    return Status::ok(Unit{});
}

const TableSchema *SchemaCatalog::find_table(std::string_view table_name) const {
    const TableStorage *storage = find_storage(table_name);
    if (storage == nullptr) {
        return nullptr;
    }

    return &storage->schema();
}

bool SchemaCatalog::table_exists(std::string_view table_name) const {
    return find_storage(table_name) != nullptr;
}

Status SchemaCatalog::validate_insert(std::string_view table_name,
                                      const std::vector<Value> &values) const {
    const TableSchema *table = find_table(table_name);
    if (table == nullptr) {
        return Status::fail(Error{"table not found"});
    }

    return validate_values(values, *table);
}

Status SchemaCatalog::insert(std::string_view table_name, Row row) {
    TableStorage *storage = find_storage(table_name);
    if (storage == nullptr) {
        return Status::fail(Error{"table not found"});
    }

#ifndef NDEBUG
    assert_row_matches_schema(row, storage->schema());
#endif

    return storage->append_row(std::move(row));
}

Status SchemaCatalog::insert_batch(std::string_view table_name, std::span<Row> rows) {
    TableStorage *storage = find_storage(table_name);
    if (storage == nullptr) {
        return Status::fail(Error{"table not found"});
    }

    if (rows.empty()) {
        return Status::ok(Unit{});
    }

#ifndef NDEBUG
    for (const Row &row : rows) {
        assert_row_matches_schema(row, storage->schema());
    }
#endif

    return storage->append_rows(rows);
}

Status SchemaCatalog::insert_columnar_batch(std::string_view table_name,
                                            const ColumnBatch &batch) {
    TableStorage *storage = find_storage(table_name);
    if (storage == nullptr) {
        return Status::fail(Error{"table not found"});
    }

    return storage->append_column_batch(batch);
}

} // namespace duradb
