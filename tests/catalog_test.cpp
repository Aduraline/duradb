#include "catalog/catalog.hpp"

#include <gtest/gtest.h>

using duradb::Catalog;
using duradb::LogicalType;
using duradb::TableSchema;

TEST(CatalogTest, CreatesTable) {
    Catalog catalog;

    TableSchema schema;
    schema.name = "users";
    schema.columns = {
        {"id", LogicalType::Int, 0},
        {"name", LogicalType::Text, 1},
    };

    ASSERT_TRUE(catalog.create_table(std::move(schema)).has_value());
    ASSERT_TRUE(catalog.table_exists("users"));

    const TableSchema *table = catalog.find_table("users");
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(table->columns.size(), 2U);
}

TEST(CatalogTest, RejectsDuplicateTable) {
    Catalog catalog;

    TableSchema schema;
    schema.name = "users";
    schema.columns = {{"id", LogicalType::Int, 0}};

    ASSERT_TRUE(catalog.create_table(schema).has_value());
    EXPECT_FALSE(catalog.create_table(std::move(schema)).has_value());
}

TEST(CatalogTest, ValidatesInsertTypes) {
    Catalog catalog;

    TableSchema schema;
    schema.name = "users";
    schema.columns = {
        {"id", LogicalType::Int, 0},
        {"name", LogicalType::Text, 1},
    };
    ASSERT_TRUE(catalog.create_table(std::move(schema)).has_value());

    EXPECT_TRUE(
        catalog
            .validate_insert("users", {duradb::Value::from_int(1), duradb::Value::from_text("Ada")})
            .has_value());
    EXPECT_FALSE(
        catalog
            .validate_insert("users", {duradb::Value::from_text("Ada"), duradb::Value::from_int(1)})
            .has_value());
}
