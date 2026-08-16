#include "engine/database_engine.hpp"
#include "storage/storage_constants.hpp"

#include <gtest/gtest.h>
#include <span>

using duradb::ColumnBatch;
using duradb::DatabaseEngine;
using duradb::LogicalType;
using duradb::Row;
using duradb::TableSchema;
using duradb::TextColumnBatchView;
using duradb::Value;

namespace {

TableSchema users_schema() {
    TableSchema schema;
    schema.name = "users";
    schema.columns = {
        {"id", LogicalType::Int, 0},
        {"name", LogicalType::Text, 1},
    };
    return schema;
}

TableSchema ids_schema() {
    TableSchema schema;
    schema.name = "ids";
    schema.columns = {{"id", LogicalType::Int, 0}};
    return schema;
}

} // namespace

TEST(StorageTest, InsertsAndScansRows) {
    DatabaseEngine engine;
    ASSERT_TRUE(engine.create_table(users_schema()).has_value());

    ASSERT_TRUE(
        engine.insert("users", Row{{Value::from_int(1), Value::from_text("Ada")}}).has_value());
    ASSERT_TRUE(
        engine.insert("users", Row{{Value::from_int(2), Value::from_text("Bob")}}).has_value());

    std::size_t row_count = 0;
    ASSERT_TRUE(engine
                    .for_each_row("users",
                                  [&](const Row &row) {
                                      ++row_count;
                                      EXPECT_FALSE(row.values.empty());
                                  })
                    .has_value());
    EXPECT_EQ(row_count, 2U);
}

TEST(StorageTest, InsertsBatch) {
    DatabaseEngine engine;
    ASSERT_TRUE(engine.create_table(users_schema()).has_value());

    Row rows[] = {
        Row{{Value::from_int(1), Value::from_text("Ada")}},
        Row{{Value::from_int(2), Value::from_text("Bob")}},
    };

    ASSERT_TRUE(engine.insert_batch("users", rows).has_value());

    std::size_t row_count = 0;
    ASSERT_TRUE(engine.for_each_row("users", [&](const Row &) { ++row_count; }).has_value());
    EXPECT_EQ(row_count, 2U);
}

TEST(StorageTest, ScanProjectedSkipsUnneededColumns) {
    DatabaseEngine engine;
    ASSERT_TRUE(engine.create_table(users_schema()).has_value());
    ASSERT_TRUE(
        engine.insert("users", Row{{Value::from_int(1), Value::from_text("Ada")}}).has_value());

    std::vector<duradb::Value> projected;
    const std::size_t name_ordinal = 1;
    ASSERT_TRUE(
        engine
            .scan_projected("users", std::span<const std::size_t>(&name_ordinal, 1),
                            [&](const std::vector<duradb::Value> &values) { projected = values; })
            .has_value());

    ASSERT_EQ(projected.size(), 1U);
    EXPECT_EQ(projected.front().as_text(), "Ada");
}

TEST(StorageTest, InsertsColumnarBatch) {
    DatabaseEngine engine;
    ASSERT_TRUE(engine.create_table(users_schema()).has_value());

    const std::int64_t ids[] = {10, 20};
    const std::uint32_t offsets[] = {0, 3, 6};
    ColumnBatch batch;
    batch.row_count = 2;
    batch.columns = {std::span<const std::int64_t>(ids),
                     TextColumnBatchView{offsets, "AdaBob"}};

    ASSERT_TRUE(engine.insert_columnar_batch("users", batch).has_value());

    std::vector<std::int64_t> scanned_ids;
    std::size_t row_count = 0;
    ASSERT_TRUE(engine
                    .for_each_row("users", [&](const Row &row) {
                        ++row_count;
                        scanned_ids.push_back(row.values[0].as_int());
                    })
                    .has_value());

    EXPECT_EQ(row_count, 2U);
    ASSERT_EQ(scanned_ids.size(), 2U);
    EXPECT_EQ(scanned_ids[0], 10);
    EXPECT_EQ(scanned_ids[1], 20);
}

TEST(StorageTest, InsertsLargeIntColumnBatch) {
    DatabaseEngine engine;
    ASSERT_TRUE(engine.create_table(ids_schema()).has_value());

    std::vector<std::int64_t> ids(10000);
    for (std::size_t index = 0; index < ids.size(); ++index) {
        ids[index] = static_cast<std::int64_t>(index);
    }

    ColumnBatch batch;
    batch.row_count = ids.size();
    batch.columns = {std::span<const std::int64_t>(ids)};

    ASSERT_TRUE(engine.insert_columnar_batch("ids", batch).has_value());

    std::int64_t sum = 0;
    ASSERT_TRUE(engine.for_each_row("ids", [&](const Row &row) { sum += row.values[0].as_int(); })
                    .has_value());
    EXPECT_EQ(sum, 49995000);
}

TEST(StorageTest, SealsRowGroupWhenCapacityReached) {
    DatabaseEngine engine;
    ASSERT_TRUE(engine.create_table(ids_schema()).has_value());

    std::vector<std::int64_t> ids(duradb::kRowGroupCapacity + 1);
    for (std::size_t index = 0; index < ids.size(); ++index) {
        ids[index] = static_cast<std::int64_t>(index);
    }

    ColumnBatch batch;
    batch.row_count = ids.size();
    batch.columns = {std::span<const std::int64_t>(ids)};

    ASSERT_TRUE(engine.insert_columnar_batch("ids", batch).has_value());

    std::size_t row_count = 0;
    ASSERT_TRUE(engine.for_each_row("ids", [&](const Row &) { ++row_count; }).has_value());
    EXPECT_EQ(row_count, ids.size());
}

TEST(StorageTest, RejectsOutOfRangeProjectedOrdinal) {
    DatabaseEngine engine;
    ASSERT_TRUE(engine.create_table(users_schema()).has_value());
    ASSERT_TRUE(
        engine.insert("users", Row{{Value::from_int(1), Value::from_text("Ada")}}).has_value());

    const std::size_t invalid_ordinal = 99;
    EXPECT_FALSE(engine
                     .scan_projected("users", std::span<const std::size_t>(&invalid_ordinal, 1),
                                     [](const std::vector<duradb::Value> &) {})
                     .has_value());
}

TEST(StorageTest, RejectsInsertIntoMissingTable) {
    DatabaseEngine engine;
    EXPECT_FALSE(engine.insert("users", Row{{Value::from_int(1)}}).has_value());
}

TEST(StorageTest, RepeatedLookupUsesStringViewFind) {
    DatabaseEngine engine;
    ASSERT_TRUE(engine.create_table(users_schema()).has_value());

    for (int index = 0; index < 1000; ++index) {
        ASSERT_TRUE(engine.table_exists("users"));
        ASSERT_NE(engine.find_table("users"), nullptr);
    }
}
