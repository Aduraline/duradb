#pragma once

#include "catalog/schema.hpp"
#include "catalog/value.hpp"
#include "common/result.hpp"
#include "storage/column_batch.hpp"
#include "storage/column_buffer.hpp"
#include "storage/row.hpp"
#include "storage/storage_constants.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace duradb {

class RowGroup {
  public:
    RowGroup() = default;
    explicit RowGroup(const TableSchema &schema);

    [[nodiscard]] std::size_t row_count() const;
    [[nodiscard]] bool is_full() const;
    [[nodiscard]] bool empty() const;

    Status append_row(const std::vector<Value> &values);
    Status append_rows(std::span<const Row> rows);
    Status append_column_batch(const ColumnBatch &batch);

    [[nodiscard]] Row materialize_row(std::size_t row_index) const;
    [[nodiscard]] Value value_at(std::size_t row_index, std::size_t column_index) const;

    [[nodiscard]] std::span<const std::int64_t> int_column_span(std::size_t column_index) const;
    [[nodiscard]] const TextColumnBuffer &text_column(std::size_t column_index) const;

  private:
    TableSchema schema_;
    std::vector<ColumnBuffer> columns_;

    Status append_validated_row(const std::vector<Value> &values);
    ColumnBuffer &column_at(std::size_t column_index);
    const ColumnBuffer &column_at(std::size_t column_index) const;
};

} // namespace duradb
