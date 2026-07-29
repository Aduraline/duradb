#include "support/engine_test_utils.hpp"

#include <gtest/gtest.h>

using duradb::BoundExpressionKind;
using duradb::BoundStatement;
using duradb::DatabaseEngine;
using duradb::test::bind_sql;

namespace {

void register_bound_create(DatabaseEngine &engine, const BoundStatement &bound) {
    ASSERT_EQ(bound.kind, BoundStatement::Kind::CreateTable);
    ASSERT_TRUE(engine.create_table(bound.create_table.schema).has_value());
}

} // namespace

TEST(BinderTest, BindsCreateInsertAndSelect) {
    DatabaseEngine engine;

    const auto create = bind_sql(engine, "CREATE TABLE users (id INT, name TEXT);");
    ASSERT_TRUE(create.has_value());
    register_bound_create(engine, create.value());

    const auto insert = bind_sql(engine, "INSERT INTO users VALUES (1, 'Alice');");
    ASSERT_TRUE(insert.has_value());
    EXPECT_EQ(insert.value().kind, BoundStatement::Kind::Insert);
    ASSERT_EQ(insert.value().insert.values.size(), 2U);
    EXPECT_EQ(insert.value().insert.values[0].as_int(), 1);
    EXPECT_EQ(insert.value().insert.values[1].as_text(), "Alice");

    const auto select = bind_sql(engine, "SELECT name FROM users WHERE id > 0;");
    ASSERT_TRUE(select.has_value());
    EXPECT_EQ(select.value().kind, BoundStatement::Kind::Select);
    ASSERT_EQ(select.value().select.column_ordinals.size(), 1U);
    EXPECT_EQ(select.value().select.column_ordinals.front(), 1U);
    ASSERT_NE(select.value().select.where, nullptr);
    EXPECT_EQ(select.value().select.where->kind, BoundExpressionKind::Comparison);
}

TEST(BinderTest, RejectsUnknownTable) {
    DatabaseEngine engine;
    EXPECT_FALSE(bind_sql(engine, "SELECT * FROM users;").has_value());
}

TEST(BinderTest, RejectsUnknownColumn) {
    DatabaseEngine engine;

    const auto create = bind_sql(engine, "CREATE TABLE users (id INT);");
    ASSERT_TRUE(create.has_value());
    register_bound_create(engine, create.value());

    EXPECT_FALSE(bind_sql(engine, "SELECT missing FROM users;").has_value());
}

TEST(BinderTest, RejectsInvalidComparisonTypes) {
    DatabaseEngine engine;

    const auto create = bind_sql(engine, "CREATE TABLE users (id INT, name TEXT);");
    ASSERT_TRUE(create.has_value());
    register_bound_create(engine, create.value());

    EXPECT_FALSE(bind_sql(engine, "SELECT name FROM users WHERE name > 0;").has_value());
}

TEST(BinderTest, RejectsLiteralWhereClause) {
    DatabaseEngine engine;

    const auto create = bind_sql(engine, "CREATE TABLE users (id INT);");
    ASSERT_TRUE(create.has_value());
    register_bound_create(engine, create.value());

    EXPECT_FALSE(bind_sql(engine, "SELECT id FROM users WHERE 1;").has_value());
}

TEST(BinderTest, RejectsBareColumnWhereClause) {
    DatabaseEngine engine;

    const auto create = bind_sql(engine, "CREATE TABLE users (id INT);");
    ASSERT_TRUE(create.has_value());
    register_bound_create(engine, create.value());

    EXPECT_FALSE(bind_sql(engine, "SELECT id FROM users WHERE id;").has_value());
}
