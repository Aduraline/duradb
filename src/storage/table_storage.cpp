#include "storage/table_storage.hpp"

#include "storage/storage_constants.hpp"

#include <algorithm>
#include <vector>

namespace duradb {

namespace {

ColumnBatch make_batch_slice(const ColumnBatch &batch, const std::size_t start,
                             const std::size_t count,
                             std::vector<std::vector<std::uint32_t>> &owned_text_offsets) {
    ColumnBatch slice;
    slice.row_count = count;
    slice.columns.reserve(batch.columns.size());

    for (std::size_t column_index = 0; column_index < batch.columns.size(); ++column_index) {
        const ColumnBatchView &column = batch.columns[column_index];

        if (std::holds_alternative<std::span<const std::int64_t>>(column)) {
            const std::span<const std::int64_t> values =
                std::get<std::span<const std::int64_t>>(column);
            slice.columns.push_back(values.subspan(start, count));
            continue;
        }

        const TextColumnBatchView text_column = std::get<TextColumnBatchView>(column);
        std::vector<std::uint32_t> &rebased_offsets = owned_text_offsets[column_index];
        rebased_offsets.resize(count + 1);

        const std::uint32_t byte_base = text_column.offsets[start];
        for (std::size_t offset_index = 0; offset_index <= count; ++offset_index) {
            rebased_offsets[offset_index] = text_column.offsets[start + offset_index] - byte_base;
        }

        slice.columns.push_back(TextColumnBatchView{
            rebased_offsets,
            text_column.bytes.substr(byte_base, text_column.offsets[start + count] - byte_base),
        });
    }

    return slice;
}

} // namespace

TableStorage::TableStorage(TableSchema schema)
    : schema_(std::move(schema)), active_(schema_) {}

const TableSchema &TableStorage::schema() const { return schema_; }

std::size_t TableStorage::row_count() const {
    std::size_t total = active_.row_count();
    for (const RowGroup &group : sealed_row_groups_) {
        total += group.row_count();
    }

    return total;
}

void TableStorage::seal_active_if_full() {
    if (!active_.is_full()) {
        return;
    }

    sealed_row_groups_.push_back(std::move(active_));
    active_ = RowGroup(schema_);
}

Status TableStorage::append_to_active(const std::vector<Value> &values) {
    if (active_.is_full()) {
        seal_active_if_full();
    }

    return active_.append_row(values);
}

Status TableStorage::append_batch_to_active(const ColumnBatch &batch) {
    std::size_t start = 0;
    std::vector<std::vector<std::uint32_t>> owned_text_offsets(batch.columns.size());

    while (start < batch.row_count) {
        if (active_.is_full()) {
            seal_active_if_full();
        }

        const std::size_t available = kRowGroupCapacity - active_.row_count();
        const std::size_t count = std::min(batch.row_count - start, available);
        const ColumnBatch slice = make_batch_slice(batch, start, count, owned_text_offsets);

        if (const Status status = active_.append_column_batch(slice); !status.has_value()) {
            return status;
        }

        start += count;
        seal_active_if_full();
    }

    return Status::ok(Unit{});
}

Status TableStorage::append_row(Row row) { return append_to_active(row.values); }

Status TableStorage::append_rows(const std::span<Row> rows) {
    for (Row &row : rows) {
        if (const Status status = append_row(std::move(row)); !status.has_value()) {
            return status;
        }
    }

    return Status::ok(Unit{});
}

Status TableStorage::append_column_batch(const ColumnBatch &batch) {
    if (const Status validation = validate_column_batch(batch, schema_); !validation.has_value()) {
        return validation;
    }

    return append_batch_to_active(batch);
}

} // namespace duradb
