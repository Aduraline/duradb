#include "engine/database_engine.hpp"

#include "catalog/catalog_constants.hpp"

namespace duradb {

Status DatabaseEngine::create_table(TableSchema schema) {
    return catalog_.create_table(kPublicSchema, std::move(schema));
}

const TableSchema *DatabaseEngine::find_table(std::string_view name) const {
    return catalog_.find_table(kPublicSchema, name);
}

bool DatabaseEngine::table_exists(std::string_view name) const {
    return catalog_.table_exists(kPublicSchema, name);
}

Status DatabaseEngine::validate_insert(std::string_view table_name,
                                       const std::vector<Value> &values) const {
    return catalog_.validate_insert(kPublicSchema, table_name, values);
}

Status DatabaseEngine::insert(std::string_view table_name, Row row) {
    return catalog_.insert(kPublicSchema, table_name, std::move(row));
}

Status DatabaseEngine::insert_batch(std::string_view table_name, std::span<Row> rows) {
    return catalog_.insert_batch(kPublicSchema, table_name, rows);
}

} // namespace duradb
