#include "storage/row_group.hpp"

#include "storage/row.hpp"

#include <string>

namespace duradb {

RowGroup::RowGroup(const TableSchema &schema) : schema_(schema) {
    columns_.reserve(schema.columns.size());
    for (const ColumnSchema &column : schema.columns) {
        columns_.push_back(make_column_buffer(column.type));

        if (column.type == LogicalType::Int) {
            std::get<IntColumnBuffer>(columns_.back()).reserve(kRowGroupCapacity);
            continue;
        }

        std::get<TextColumnBuffer>(columns_.back())
            .reserve(kRowGroupCapacity, kRowGroupCapacity * 16);
    }
}

std::size_t RowGroup::row_count() const {
    if (columns_.empty()) {
        return 0;
    }

    return std::visit([](const auto &column) { return column.size(); }, columns_.front());
}

bool RowGroup::is_full() const {
    return row_count() >= kRowGroupCapacity;
}

bool RowGroup::empty() const {
    return row_count() == 0;
}

ColumnBuffer &RowGroup::column_at(const std::size_t column_index) {
    return columns_.at(column_index);
}

const ColumnBuffer &RowGroup::column_at(const std::size_t column_index) const {
    return columns_.at(column_index);
}

Status RowGroup::append_validated_row(const std::vector<Value> &values) {
    for (std::size_t column_index = 0; column_index < values.size(); ++column_index) {
        ColumnBuffer &column = column_at(column_index);
        const Value &value = values[column_index];

        if (value.type == LogicalType::Int) {
            std::get<IntColumnBuffer>(column).append(value.as_int());
            continue;
        }

        std::get<TextColumnBuffer>(column).append(value.as_text());
    }

    return Status::ok(Unit{});
}

Status RowGroup::append_row(const std::vector<Value> &values) {
    if (is_full()) {
        return Status::fail(Error{"row group is full"});
    }

    if (const Status validation = validate_values(values, schema_); !validation.has_value()) {
        return validation;
    }

    return append_validated_row(values);
}

Status RowGroup::append_rows(const std::span<const Row> rows) {
    for (const Row &row : rows) {
        if (const Status status = append_row(row.values); !status.has_value()) {
            return status;
        }
    }

    return Status::ok(Unit{});
}

Status RowGroup::append_column_batch(const ColumnBatch &batch) {
    if (is_full()) {
        return Status::fail(Error{"row group is full"});
    }

    if (const Status validation = validate_column_batch(batch, schema_); !validation.has_value()) {
        return validation;
    }

    const std::size_t available = kRowGroupCapacity - row_count();
    if (batch.row_count > available) {
        return Status::fail(Error{"column batch exceeds row group capacity"});
    }

    for (std::size_t column_index = 0; column_index < batch.columns.size(); ++column_index) {
        ColumnBuffer &column = column_at(column_index);
        const ColumnBatchView &batch_column = batch.columns[column_index];

        if (std::holds_alternative<std::span<const std::int64_t>>(batch_column)) {
            const std::span<const std::int64_t> values =
                std::get<std::span<const std::int64_t>>(batch_column);
            std::get<IntColumnBuffer>(column).append_span(values.subspan(0, batch.row_count));
            continue;
        }

        const TextColumnBatchView text_column = std::get<TextColumnBatchView>(batch_column);
        std::get<TextColumnBuffer>(column).append_batch(text_column.offsets, text_column.bytes,
                                                        batch.row_count);
    }

    return Status::ok(Unit{});
}

Value RowGroup::value_at(const std::size_t row_index, const std::size_t column_index) const {
    const ColumnBuffer &column = column_at(column_index);
    const LogicalType type = schema_.columns[column_index].type;

    if (type == LogicalType::Int) {
        return Value::from_int(std::get<IntColumnBuffer>(column).at(row_index));
    }

    return Value::from_text(std::string(std::get<TextColumnBuffer>(column).at(row_index)));
}

Row RowGroup::materialize_row(const std::size_t row_index) const {
    Row row;
    row.values.reserve(schema_.columns.size());

    for (std::size_t column_index = 0; column_index < schema_.columns.size(); ++column_index) {
        row.values.push_back(value_at(row_index, column_index));
    }

    return row;
}

std::span<const std::int64_t> RowGroup::int_column_span(const std::size_t column_index) const {
    return std::get<IntColumnBuffer>(column_at(column_index)).span();
}

const TextColumnBuffer &RowGroup::text_column(const std::size_t column_index) const {
    return std::get<TextColumnBuffer>(column_at(column_index));
}

} // namespace duradb
