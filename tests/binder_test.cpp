#include "support/catalog_test_utils.hpp"

#include <gtest/gtest.h>

using duradb::BoundStatement;
using duradb::Catalog;
using duradb::test::bind_sql;

namespace {

void register_bound_create(Catalog &catalog, const BoundStatement &bound) {
    ASSERT_EQ(bound.kind, BoundStatement::Kind::CreateTable);
    ASSERT_TRUE(catalog.create_table(bound.create_table.schema).has_value());
}

} // namespace

TEST(BinderTest, BindsCreateInsertAndSelect) {
    Catalog catalog;

    const auto create = bind_sql(catalog, "CREATE TABLE users (id INT, name TEXT);");
    ASSERT_TRUE(create.has_value());
    register_bound_create(catalog, create.value());

    const auto insert = bind_sql(catalog, "INSERT INTO users VALUES (1, 'Alice');");
    ASSERT_TRUE(insert.has_value());
    EXPECT_EQ(insert.value().kind, BoundStatement::Kind::Insert);
    ASSERT_EQ(insert.value().insert.values.size(), 2U);
    EXPECT_EQ(insert.value().insert.values[0].int_value, 1);
    EXPECT_EQ(insert.value().insert.values[1].text_value, "Alice");

    const auto select = bind_sql(catalog, "SELECT name FROM users WHERE id > 0;");
    ASSERT_TRUE(select.has_value());
    EXPECT_EQ(select.value().kind, BoundStatement::Kind::Select);
    ASSERT_EQ(select.value().select.column_ordinals.size(), 1U);
    EXPECT_EQ(select.value().select.column_ordinals.front(), 1U);
    ASSERT_NE(select.value().select.where, nullptr);
}

TEST(BinderTest, RejectsUnknownTable) {
    Catalog catalog;
    EXPECT_FALSE(bind_sql(catalog, "SELECT * FROM users;").has_value());
}

TEST(BinderTest, RejectsUnknownColumn) {
    Catalog catalog;

    const auto create = bind_sql(catalog, "CREATE TABLE users (id INT);");
    ASSERT_TRUE(create.has_value());
    register_bound_create(catalog, create.value());

    EXPECT_FALSE(bind_sql(catalog, "SELECT missing FROM users;").has_value());
}
