#pragma once

#include "catalog/schema.hpp"
#include "catalog/value.hpp"
#include "common/result.hpp"
#include "common/string_view_hash.hpp"
#include "storage/column_batch.hpp"
#include "storage/row.hpp"
#include "storage/table_storage.hpp"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace duradb {

class SchemaCatalog {
  public:
    explicit SchemaCatalog(std::string name);

    std::string_view name() const;

    Status create_table(TableSchema schema);

    const TableSchema *find_table(std::string_view table_name) const;
    bool table_exists(std::string_view table_name) const;

    std::vector<std::string> table_names() const;

    Status validate_insert(std::string_view table_name, const std::vector<Value> &values) const;

    Status insert(std::string_view table_name, Row row);
    Status insert_batch(std::string_view table_name, std::span<Row> rows);
    Status insert_columnar_batch(std::string_view table_name, const ColumnBatch &batch);

    template <typename RowFn>
    Status for_each_row(std::string_view table_name, RowFn &&row_fn) const {
        const TableStorage *storage = find_storage(table_name);
        if (storage == nullptr) {
            return Status::fail(Error{"table not found"});
        }

        return storage->for_each_row(std::forward<RowFn>(row_fn));
    }

    template <typename ProjectedRowFn>
    Status scan_projected(std::string_view table_name, std::span<const std::size_t> column_ordinals,
                          ProjectedRowFn &&projected_row_fn) const {
        const TableStorage *storage = find_storage(table_name);
        if (storage == nullptr) {
            return Status::fail(Error{"table not found"});
        }

        return storage->scan_projected(column_ordinals, std::forward<ProjectedRowFn>(projected_row_fn));
    }

  private:
    TableStorage *find_storage(std::string_view table_name);
    const TableStorage *find_storage(std::string_view table_name) const;

    std::string name_;
    std::unordered_map<std::string, TableStorage, StringViewHash, StringViewEqual> tables_;
};

} // namespace duradb
