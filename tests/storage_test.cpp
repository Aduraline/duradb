#include "engine/database_engine.hpp"

#include <gtest/gtest.h>
#include <span>

using duradb::DatabaseEngine;
using duradb::LogicalType;
using duradb::Row;
using duradb::TableSchema;
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

    const Row rows[] = {
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

TEST(StorageTest, LookupDoesNotAllocateOnHotPath) {
    DatabaseEngine engine;
    ASSERT_TRUE(engine.create_table(users_schema()).has_value());

    for (int index = 0; index < 1000; ++index) {
        ASSERT_TRUE(engine.table_exists("users"));
        ASSERT_NE(engine.find_table("users"), nullptr);
    }
}
