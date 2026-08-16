#pragma once

#include "catalog/database_catalog.hpp"
#include "catalog/schema.hpp"
#include "catalog/value.hpp"
#include "common/result.hpp"
#include "storage/column_batch.hpp"
#include "storage/row.hpp"

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace duradb {

// Single-database facade over DatabaseCatalog for tests and legacy call sites.
class DatabaseEngine {
  public:
    Status create_table(TableSchema schema);

    const TableSchema *find_table(std::string_view name) const;
    bool table_exists(std::string_view name) const;

    Status validate_insert(std::string_view table_name, const std::vector<Value> &values) const;

    Status insert(std::string_view table_name, Row row);
    Status insert_batch(std::string_view table_name, std::span<Row> rows);
    Status insert_columnar_batch(std::string_view table_name, const ColumnBatch &batch);

    template <typename RowFn>
    Status for_each_row(std::string_view table_name, RowFn &&row_fn) const {
        return catalog_.for_each_row(kPublicSchema, table_name, std::forward<RowFn>(row_fn));
    }

    template <typename ProjectedRowFn>
    Status scan_projected(std::string_view table_name, std::span<const std::size_t> column_ordinals,
                          ProjectedRowFn &&projected_row_fn) const {
        return catalog_.scan_projected(kPublicSchema, table_name, column_ordinals,
                                       std::forward<ProjectedRowFn>(projected_row_fn));
    }

    DatabaseCatalog &catalog() { return catalog_; }
    const DatabaseCatalog &catalog() const { return catalog_; }

  private:
    DatabaseCatalog catalog_;
};

} // namespace duradb
