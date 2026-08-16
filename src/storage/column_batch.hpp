#pragma once

#include "catalog/schema.hpp"
#include "common/result.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

namespace duradb {

struct TextColumnBatchView {
    std::span<const std::uint32_t> offsets;
    std::string_view bytes;
};

using ColumnBatchView = std::variant<std::span<const std::int64_t>, TextColumnBatchView>;

struct ColumnBatch {
    std::size_t row_count = 0;
    std::vector<ColumnBatchView> columns;
};

[[nodiscard]] Status validate_column_batch(const ColumnBatch &batch, const TableSchema &schema);

} // namespace duradb
