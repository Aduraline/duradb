#include "storage/row.hpp"

#include <gtest/gtest.h>

using duradb::LogicalType;
using duradb::Row;
using duradb::TableSchema;

TEST(RowTest, ValidatesMatchingRow) {
    TableSchema schema;
    schema.name = "users";
    schema.columns = {
        {"id", LogicalType::Int, 0},
        {"name", LogicalType::Text, 1},
    };

    Row row{{duradb::Value::from_int(1), duradb::Value::from_text("Ada")}};
    EXPECT_TRUE(validate_row(row, schema).has_value());
}

TEST(RowTest, RejectsColumnCountMismatch) {
    TableSchema schema;
    schema.name = "users";
    schema.columns = {{"id", LogicalType::Int, 0}};

    Row row{{duradb::Value::from_int(1), duradb::Value::from_text("Ada")}};
    EXPECT_FALSE(validate_row(row, schema).has_value());
}

TEST(RowTest, RejectsPayloadMismatch) {
    TableSchema schema;
    schema.name = "users";
    schema.columns = {{"id", LogicalType::Int, 0}};

    duradb::Value value;
    value.type = LogicalType::Int;
    value.payload = std::string("not-an-int");

    Row row{{value}};
    EXPECT_FALSE(validate_row(row, schema).has_value());
}

TEST(RowTest, RejectsTypeMismatch) {
    TableSchema schema;
    schema.name = "users";
    schema.columns = {{"id", LogicalType::Int, 0}};

    Row row{{duradb::Value::from_text("not-an-int")}};
    EXPECT_FALSE(validate_row(row, schema).has_value());
}
