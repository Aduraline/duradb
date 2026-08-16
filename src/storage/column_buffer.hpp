#pragma once

#include "frontend/ast.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace duradb {

class IntColumnBuffer {
  public:
    void reserve(std::size_t row_count);
    void append(std::int64_t value);
    void append_span(std::span<const std::int64_t> values);

    [[nodiscard]] std::int64_t at(std::size_t index) const;
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] std::span<const std::int64_t> span() const;

  private:
    std::vector<std::int64_t> values_;
};

class TextColumnBuffer {
  public:
    void reserve(std::size_t row_count, std::size_t byte_hint);
    void append(std::string_view value);
    void append_batch(std::span<const std::uint32_t> offsets, std::string_view bytes,
                      std::size_t row_count);

    [[nodiscard]] std::string_view at(std::size_t index) const;
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] std::span<const std::uint32_t> offsets_span() const;
    [[nodiscard]] std::string_view bytes() const;

  private:
    std::vector<std::uint32_t> offsets_;
    std::string bytes_;
};

using ColumnBuffer = std::variant<IntColumnBuffer, TextColumnBuffer>;

[[nodiscard]] ColumnBuffer make_column_buffer(LogicalType type);

} // namespace duradb
