#include "storage/heap_table.hpp"

#include <gtest/gtest.h>

using duradb::HeapTable;
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

TEST(HeapTableTest, CreatesTable) {
    HeapTable storage;
    EXPECT_TRUE(storage.create_table(users_schema()).has_value());
    EXPECT_TRUE(storage.table_exists("users"));
}

TEST(HeapTableTest, InsertsAndScansRows) {
    HeapTable storage;
    ASSERT_TRUE(storage.create_table(users_schema()).has_value());

    ASSERT_TRUE(
        storage.insert("users", Row{{Value::from_int(1), Value::from_text("Ada")}}).has_value());
    ASSERT_TRUE(
        storage.insert("users", Row{{Value::from_int(2), Value::from_text("Bob")}}).has_value());

    const auto rows = storage.scan("users");
    ASSERT_TRUE(rows.has_value());
    ASSERT_EQ(rows.value().size(), 2U);
    EXPECT_EQ(rows.value()[0].values[0].int_value, 1);
    EXPECT_EQ(rows.value()[1].values[1].text_value, "Bob");
}

TEST(HeapTableTest, RejectsInsertIntoMissingTable) {
    HeapTable storage;
    EXPECT_FALSE(storage.insert("users", Row{{Value::from_int(1)}}).has_value());
}

TEST(HeapTableTest, RejectsDuplicateTable) {
    HeapTable storage;
    ASSERT_TRUE(storage.create_table(users_schema()).has_value());
    EXPECT_FALSE(storage.create_table(users_schema()).has_value());
}
