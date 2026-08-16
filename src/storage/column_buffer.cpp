#include "storage/column_buffer.hpp"

#include <cassert>

namespace duradb {

void IntColumnBuffer::reserve(const std::size_t row_count) {
    values_.reserve(row_count);
}

void IntColumnBuffer::append(const std::int64_t value) {
    values_.push_back(value);
}

void IntColumnBuffer::append_span(const std::span<const std::int64_t> values) {
    values_.reserve(values_.size() + values.size());
    values_.insert(values_.end(), values.begin(), values.end());
}

std::int64_t IntColumnBuffer::at(const std::size_t index) const {
    return values_.at(index);
}

std::size_t IntColumnBuffer::size() const {
    return values_.size();
}

std::span<const std::int64_t> IntColumnBuffer::span() const {
    return values_;
}

void TextColumnBuffer::reserve(const std::size_t row_count, const std::size_t byte_hint) {
    offsets_.reserve(row_count);
    bytes_.reserve(byte_hint);
}

void TextColumnBuffer::append(const std::string_view value) {
    offsets_.push_back(static_cast<std::uint32_t>(bytes_.size()));
    bytes_.append(value);
}

void TextColumnBuffer::append_batch(const std::span<const std::uint32_t> offsets,
                                    const std::string_view bytes, const std::size_t row_count) {
    assert(row_count > 0);
    assert(offsets.size() == row_count + 1);
    assert(offsets.back() <= bytes.size());

    const std::size_t base_offset = bytes_.size();
    offsets_.reserve(offsets_.size() + row_count);
    bytes_.reserve(bytes_.size() + bytes.size());
    bytes_.append(bytes);

    for (std::size_t row_index = 0; row_index < row_count; ++row_index) {
        offsets_.push_back(base_offset + offsets[row_index]);
    }
}

std::string_view TextColumnBuffer::at(const std::size_t index) const {
    const std::uint32_t start = offsets_.at(index);
    const std::uint32_t end = index + 1 < offsets_.size()
                                  ? offsets_.at(index + 1)
                                  : static_cast<std::uint32_t>(bytes_.size());
    return std::string_view(bytes_.data() + start, end - start);
}

std::size_t TextColumnBuffer::size() const {
    return offsets_.size();
}

std::span<const std::uint32_t> TextColumnBuffer::offsets_span() const {
    return offsets_;
}

std::string_view TextColumnBuffer::bytes() const {
    return bytes_;
}

ColumnBuffer make_column_buffer(const LogicalType type) {
    switch (type) {
    case LogicalType::Int:
        return IntColumnBuffer{};
    case LogicalType::Text:
        return TextColumnBuffer{};
    }

    assert(false && "unsupported logical type");
    return IntColumnBuffer{};
}

} // namespace duradb
