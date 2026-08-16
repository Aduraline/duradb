#pragma once

#include "catalog/catalog_constants.hpp"
#include "catalog/schema_catalog.hpp"
#include "common/string_view_hash.hpp"

#include <span>
#include <string>
#include <string_view>

namespace duradb {

class DatabaseCatalog {
  public:
    DatabaseCatalog();

    Status create_schema(std::string name);

    bool schema_exists(std::string_view schema_name) const;
    const SchemaCatalog *find_schema(std::string_view schema_name) const;

    Status create_table(std::string_view schema_name, TableSchema schema);

    const TableSchema *find_table(std::string_view schema_name, std::string_view table_name) const;
    bool table_exists(std::string_view schema_name, std::string_view table_name) const;

    Status validate_insert(std::string_view schema_name, std::string_view table_name,
                           const std::vector<Value> &values) const;

    Status insert(std::string_view schema_name, std::string_view table_name, Row row);
    Status insert_batch(std::string_view schema_name, std::string_view table_name,
                        std::span<Row> rows);
    Status insert_columnar_batch(std::string_view schema_name, std::string_view table_name,
                                 const ColumnBatch &batch);

    template <typename RowFn>
    Status for_each_row(std::string_view schema_name, std::string_view table_name,
                        RowFn &&row_fn) const {
        const SchemaCatalog *schema = find_schema(schema_name);
        if (schema == nullptr) {
            return Status::fail(Error{"schema not found"});
        }

        return schema->for_each_row(table_name, std::forward<RowFn>(row_fn));
    }

    template <typename ProjectedRowFn>
    Status scan_projected(std::string_view schema_name, std::string_view table_name,
                          std::span<const std::size_t> column_ordinals,
                          ProjectedRowFn &&projected_row_fn) const {
        const SchemaCatalog *schema = find_schema(schema_name);
        if (schema == nullptr) {
            return Status::fail(Error{"schema not found"});
        }

        return schema->scan_projected(table_name, column_ordinals,
                                      std::forward<ProjectedRowFn>(projected_row_fn));
    }

  private:
    SchemaCatalog *find_schema_mut(std::string_view schema_name);

    std::unordered_map<std::string, SchemaCatalog, StringViewHash, StringViewEqual> schemas_;
};

} // namespace duradb
