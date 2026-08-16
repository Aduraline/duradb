#pragma once

#include "catalog/schema.hpp"
#include "catalog/value.hpp"
#include "common/result.hpp"
#include "storage/column_batch.hpp"
#include "storage/row.hpp"
#include "storage/row_group.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace duradb {

class TableStorage {
  public:
    TableStorage() = default;
    explicit TableStorage(TableSchema schema);

    [[nodiscard]] const TableSchema &schema() const;
    [[nodiscard]] std::size_t row_count() const;

    Status append_row(Row row);
    Status append_rows(std::span<Row> rows);
    Status append_column_batch(const ColumnBatch &batch);

    template <typename RowFn>
    Status for_each_row(RowFn &&row_fn) const {
        const auto visit_group = [&](const RowGroup &group) -> Status {
            for (std::size_t row_index = 0; row_index < group.row_count(); ++row_index) {
                row_fn(group.materialize_row(row_index));
            }

            return Status::ok(Unit{});
        };

        for (const RowGroup &group : sealed_row_groups_) {
            if (const Status status = visit_group(group); !status.has_value()) {
                return status;
            }
        }

        return visit_group(active_);
    }

    template <typename ProjectedRowFn>
    Status scan_projected(std::span<const std::size_t> column_ordinals,
                          ProjectedRowFn &&projected_row_fn) const {
        const std::size_t column_count = schema_.columns.size();

        for (const std::size_t ordinal : column_ordinals) {
            if (ordinal >= column_count) {
                return Status::fail(Error{"column ordinal out of range"});
            }
        }

        const auto scan_group = [&](const RowGroup &group) {
            for (std::size_t row_index = 0; row_index < group.row_count(); ++row_index) {
                std::vector<Value> projected;
                projected.reserve(column_ordinals.size());

                for (const std::size_t ordinal : column_ordinals) {
                    projected.push_back(group.value_at(row_index, ordinal));
                }

                projected_row_fn(projected);
            }
        };

        for (const RowGroup &group : sealed_row_groups_) {
            scan_group(group);
        }

        scan_group(active_);
        return Status::ok(Unit{});
    }

  private:
    TableSchema schema_;
    std::vector<RowGroup> sealed_row_groups_;
    RowGroup active_;

    Status append_to_active(const std::vector<Value> &values);
    Status append_batch_to_active(const ColumnBatch &batch);
    void seal_active_if_full();
};

} // namespace duradb
