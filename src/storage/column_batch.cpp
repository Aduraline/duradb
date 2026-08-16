#include "storage/column_batch.hpp"

namespace duradb {

namespace {

bool text_offsets_are_monotonic(const TextColumnBatchView &text_column) {
    for (std::size_t index = 1; index < text_column.offsets.size(); ++index) {
        if (text_column.offsets[index] < text_column.offsets[index - 1]) {
            return false;
        }
    }

    return true;
}

} // namespace

Status validate_column_batch(const ColumnBatch &batch, const TableSchema &schema) {
    if (batch.columns.size() != schema.columns.size()) {
        return Status::fail(Error{"column batch width mismatch"});
    }

    for (std::size_t column_index = 0; column_index < schema.columns.size(); ++column_index) {
        const LogicalType expected_type = schema.columns[column_index].type;
        const ColumnBatchView &column = batch.columns[column_index];

        if (expected_type == LogicalType::Int) {
            if (!std::holds_alternative<std::span<const std::int64_t>>(column)) {
                return Status::fail(Error{"column batch type mismatch"});
            }

            if (std::get<std::span<const std::int64_t>>(column).size() < batch.row_count) {
                return Status::fail(Error{"int column batch too short"});
            }

            continue;
        }

        if (expected_type == LogicalType::Text) {
            if (!std::holds_alternative<TextColumnBatchView>(column)) {
                return Status::fail(Error{"column batch type mismatch"});
            }

            const TextColumnBatchView text_column = std::get<TextColumnBatchView>(column);
            if (text_column.offsets.size() != batch.row_count + 1) {
                return Status::fail(Error{"text column offsets must be row_count + 1"});
            }

            if (text_column.offsets.front() != 0) {
                return Status::fail(Error{"text column offsets must start at zero"});
            }

            if (text_column.offsets.back() > text_column.bytes.size()) {
                return Status::fail(Error{"text column offsets out of range"});
            }

            if (!text_offsets_are_monotonic(text_column)) {
                return Status::fail(Error{"text column offsets must be monotonic"});
            }

            continue;
        }

        return Status::fail(Error{"unsupported column type"});
    }

    return Status::ok(Unit{});
}

} // namespace duradb
