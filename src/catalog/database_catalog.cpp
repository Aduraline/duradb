#include "catalog/database_catalog.hpp"

#include "catalog/catalog_constants.hpp"
#include "storage/column_batch.hpp"

namespace duradb {

DatabaseCatalog::DatabaseCatalog() {
    schemas_.emplace(std::string(kPublicSchema), SchemaCatalog{std::string(kPublicSchema)});
}

Status DatabaseCatalog::create_schema(std::string name) {
    if (name.empty()) {
        return Status::fail(Error{"schema name must not be empty"});
    }

    if (schemas_.contains(name)) {
        return Status::fail(Error{"schema already exists"});
    }

    schemas_.emplace(name, SchemaCatalog{name});
    return Status::ok(Unit{});
}

bool DatabaseCatalog::schema_exists(std::string_view schema_name) const {
    return find_schema(schema_name) != nullptr;
}

SchemaCatalog *DatabaseCatalog::find_schema_mut(std::string_view schema_name) {
    const auto iterator = schemas_.find(schema_name);
    if (iterator == schemas_.end()) {
        return nullptr;
    }

    return &iterator->second;
}

const SchemaCatalog *DatabaseCatalog::find_schema(std::string_view schema_name) const {
    const auto iterator = schemas_.find(schema_name);
    if (iterator == schemas_.end()) {
        return nullptr;
    }

    return &iterator->second;
}

Status DatabaseCatalog::create_table(std::string_view schema_name, TableSchema schema) {
    SchemaCatalog *schema_catalog = find_schema_mut(schema_name);
    if (schema_catalog == nullptr) {
        return Status::fail(Error{"schema not found"});
    }

    return schema_catalog->create_table(std::move(schema));
}

const TableSchema *DatabaseCatalog::find_table(std::string_view schema_name,
                                               std::string_view table_name) const {
    const SchemaCatalog *schema_catalog = find_schema(schema_name);
    if (schema_catalog == nullptr) {
        return nullptr;
    }

    return schema_catalog->find_table(table_name);
}

bool DatabaseCatalog::table_exists(std::string_view schema_name,
                                   std::string_view table_name) const {
    return find_table(schema_name, table_name) != nullptr;
}

Status DatabaseCatalog::validate_insert(std::string_view schema_name, std::string_view table_name,
                                        const std::vector<Value> &values) const {
    const SchemaCatalog *schema_catalog = find_schema(schema_name);
    if (schema_catalog == nullptr) {
        return Status::fail(Error{"schema not found"});
    }

    return schema_catalog->validate_insert(table_name, values);
}

Status DatabaseCatalog::insert(std::string_view schema_name, std::string_view table_name, Row row) {
    SchemaCatalog *schema_catalog = find_schema_mut(schema_name);
    if (schema_catalog == nullptr) {
        return Status::fail(Error{"schema not found"});
    }

    return schema_catalog->insert(table_name, std::move(row));
}

Status DatabaseCatalog::insert_batch(std::string_view schema_name, std::string_view table_name,
                                     std::span<Row> rows) {
    SchemaCatalog *schema_catalog = find_schema_mut(schema_name);
    if (schema_catalog == nullptr) {
        return Status::fail(Error{"schema not found"});
    }

    return schema_catalog->insert_batch(table_name, rows);
}

Status DatabaseCatalog::insert_columnar_batch(std::string_view schema_name,
                                              std::string_view table_name,
                                              const ColumnBatch &batch) {
    SchemaCatalog *schema_catalog = find_schema_mut(schema_name);
    if (schema_catalog == nullptr) {
        return Status::fail(Error{"schema not found"});
    }

    return schema_catalog->insert_columnar_batch(table_name, batch);
}

} // namespace duradb
