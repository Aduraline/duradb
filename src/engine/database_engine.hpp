#pragma once

#include "catalog/schema.hpp"
#include "catalog/value.hpp"
#include "common/result.hpp"
#include "common/string_view_hash.hpp"
#include "storage/row.hpp"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace duradb {

class DatabaseEngine {
  public:
    Status create_table(TableSchema schema);

    const TableSchema *find_table(std::string_view name) const;
    bool table_exists(std::string_view name) const;

    Status validate_insert(std::string_view table_name, const std::vector<Value> &values) const;

    Status insert(std::string_view table_name, Row row);
    Status insert_batch(std::string_view table_name, std::span<Row> rows);

    template <typename RowFn>
    Status for_each_row(std::string_view table_name, RowFn &&row_fn) const {
        const TableStorage *storage = find_storage(table_name);
        if (storage == nullptr) {
            return Status::fail(Error{"table not found"});
        }

        for (const Row &row : storage->rows) {
            row_fn(row);
        }

        return Status::ok(Unit{});
    }

    template <typename ProjectedRowFn>
    Status scan_projected(std::string_view table_name, std::span<const std::size_t> column_ordinals,
                          ProjectedRowFn &&projected_row_fn) const {
        const TableStorage *storage = find_storage(table_name);
        if (storage == nullptr) {
            return Status::fail(Error{"table not found"});
        }

        const std::size_t column_count = storage->schema.columns.size();

        for (const std::size_t ordinal : column_ordinals) {
            if (ordinal >= column_count) {
                return Status::fail(Error{"column ordinal out of range"});
            }
        }

        // TODO: pass row and ordinals to callback to avoid per-row allocation
        for (const Row &row : storage->rows) {
            std::vector<Value> projected;
            projected.reserve(column_ordinals.size());

            for (const std::size_t ordinal : column_ordinals) {
                projected.push_back(row.values[ordinal]);
            }

            projected_row_fn(projected);
        }

        return Status::ok(Unit{});
    }

  private:
    struct TableStorage {
        TableSchema schema;
        std::vector<Row> rows; // TODO: slotted pages via buffer pool
    };

    TableStorage *find_storage(std::string_view table_name);
    const TableStorage *find_storage(std::string_view table_name) const;

    std::unordered_map<std::string, TableStorage, StringViewHash, StringViewEqual>
        tables_; // TODO: durable heap file on disk
    // TODO: column statistics for read-optimised planning
};

} // namespace duradb
