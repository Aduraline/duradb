#include "engine/database_engine.hpp"

#include <gtest/gtest.h>

using duradb::DatabaseEngine;
using duradb::LogicalType;
using duradb::TableSchema;
using duradb::Value;

TEST(EngineTest, CreatesTable) {
    DatabaseEngine engine;

    TableSchema schema;
    schema.name = "users";
    schema.columns = {
        {"id", LogicalType::Int, 0},
        {"name", LogicalType::Text, 1},
    };

    ASSERT_TRUE(engine.create_table(std::move(schema)).has_value());
    ASSERT_TRUE(engine.table_exists("users"));

    const TableSchema *table = engine.find_table("users");
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(table->columns.size(), 2U);
}

TEST(EngineTest, RejectsDuplicateTable) {
    DatabaseEngine engine;

    TableSchema schema;
    schema.name = "users";
    schema.columns = {{"id", LogicalType::Int, 0}};

    ASSERT_TRUE(engine.create_table(schema).has_value());
    EXPECT_FALSE(engine.create_table(std::move(schema)).has_value());
}

TEST(EngineTest, ValidatesInsertTypes) {
    DatabaseEngine engine;

    TableSchema schema;
    schema.name = "users";
    schema.columns = {
        {"id", LogicalType::Int, 0},
        {"name", LogicalType::Text, 1},
    };
    ASSERT_TRUE(engine.create_table(std::move(schema)).has_value());

    EXPECT_TRUE(
        engine.validate_insert("users", {Value::from_int(1), Value::from_text("Ada")}).has_value());
    EXPECT_FALSE(
        engine.validate_insert("users", {Value::from_text("Ada"), Value::from_int(1)}).has_value());
}
