#include "support/engine_test_utils.hpp"

#include <gtest/gtest.h>

using duradb::BoundExpressionKind;
using duradb::DatabaseEngine;
using duradb::test::bind_sql;

TEST(BoundExprTest, BindsAndExpression) {
    DatabaseEngine engine;

    const auto create = bind_sql(engine, "CREATE TABLE users (id INT, name TEXT);");
    ASSERT_TRUE(create.has_value());
    ASSERT_TRUE(engine.create_table(create.value().create_table.schema).has_value());

    const auto select = bind_sql(engine, "SELECT name FROM users WHERE id > 0 AND name = 'Ada';");
    ASSERT_TRUE(select.has_value());
    ASSERT_NE(select.value().select.where, nullptr);
    EXPECT_EQ(select.value().select.where->kind, BoundExpressionKind::And);
}

TEST(BoundExprTest, RejectsTextOrderingComparison) {
    DatabaseEngine engine;

    const auto create = bind_sql(engine, "CREATE TABLE users (name TEXT);");
    ASSERT_TRUE(create.has_value());
    ASSERT_TRUE(engine.create_table(create.value().create_table.schema).has_value());

    EXPECT_FALSE(bind_sql(engine, "SELECT name FROM users WHERE name > 'Ada';").has_value());
}
