#include "catalog/schema.hpp"
#include "catalog/value.hpp"
#include "storage/column_batch.hpp"
#include "storage/column_buffer.hpp"
#include "storage/row.hpp"
#include "storage/row_group.hpp"

#include <gtest/gtest.h>

using duradb::IntColumnBuffer;
using duradb::LogicalType;
using duradb::RowGroup;
using duradb::TableSchema;
using duradb::TextColumnBatchView;
using duradb::TextColumnBuffer;
using duradb::Value;

namespace {

TableSchema metrics_schema() {
    TableSchema schema;
    schema.name = "metrics";
    schema.columns = {
        {"id", LogicalType::Int, 0},
        {"name", LogicalType::Text, 1},
    };
    return schema;
}

} // namespace

TEST(RowGroupTest, StoresFixedWidthIntsAsContiguousArray) {
    IntColumnBuffer column;
    column.append(10);
    column.append(20);
    column.append(30);

    ASSERT_EQ(column.size(), 3U);
    EXPECT_EQ(column.at(1), 20);

    const std::span<const std::int64_t> values = column.span();
    EXPECT_EQ(values.size(), 3U);
    EXPECT_EQ(values[0], 10);
}

TEST(RowGroupTest, StoresTextAsOffsetsAndBytesBlob) {
    TextColumnBuffer column;
    column.append("Ada");
    column.append("Bob");

    ASSERT_EQ(column.size(), 2U);
    EXPECT_EQ(column.at(0), "Ada");
    EXPECT_EQ(column.at(1), "Bob");
    EXPECT_EQ(column.bytes(), "AdaBob");
}

TEST(RowGroupTest, AppendsRowsIntoColumnBuffers) {
    RowGroup group(metrics_schema());

    ASSERT_TRUE(group.append_row({Value::from_int(1), Value::from_text("Ada")}).has_value());
    ASSERT_TRUE(group.append_row({Value::from_int(2), Value::from_text("Bob")}).has_value());

    EXPECT_EQ(group.row_count(), 2U);
    EXPECT_EQ(group.int_column_span(0)[1], 2);
    EXPECT_EQ(group.text_column(1).at(0), "Ada");
}

TEST(RowGroupTest, MaterializesRowForExecutorCompatibility) {
    RowGroup group(metrics_schema());
    ASSERT_TRUE(group.append_row({Value::from_int(42), Value::from_text("Dura")}).has_value());

    const duradb::Row row = group.materialize_row(0);
    ASSERT_EQ(row.values.size(), 2U);
    EXPECT_EQ(row.values[0].as_int(), 42);
    EXPECT_EQ(row.values[1].as_text(), "Dura");
}

TEST(RowGroupTest, AppendsColumnBatchWithoutRowObjects) {
    RowGroup group(metrics_schema());

    const std::int64_t ids[] = {1, 2, 3};
    const std::uint32_t offsets[] = {0, 3, 6, 9};
    const TextColumnBatchView name_column{offsets, "AdaBobEve"};

    duradb::ColumnBatch batch;
    batch.row_count = 3;
    batch.columns = {std::span<const std::int64_t>(ids), name_column};

    ASSERT_TRUE(group.append_column_batch(batch).has_value());
    EXPECT_EQ(group.row_count(), 3U);
    EXPECT_EQ(group.int_column_span(0)[2], 3);
    EXPECT_EQ(group.text_column(1).at(2), "Eve");
}

TEST(RowGroupTest, RejectsNonMonotonicTextOffsets) {
    RowGroup group(metrics_schema());

    const std::int64_t ids[] = {1};
    const std::uint32_t offsets[] = {0, 5, 3};
    const TextColumnBatchView name_column{offsets, "AdaBob"};

    duradb::ColumnBatch batch;
    batch.row_count = 2;
    batch.columns = {std::span<const std::int64_t>(ids), name_column};

    EXPECT_FALSE(group.append_column_batch(batch).has_value());
}
