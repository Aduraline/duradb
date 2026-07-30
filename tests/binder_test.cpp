#include "support/engine_test_utils.hpp"

#include "execution/executor.hpp"

#include <gtest/gtest.h>

using duradb::BoundExpressionKind;
using duradb::BoundStatement;
using duradb::Cluster;
using duradb::Executor;
using duradb::Session;
using duradb::test::bind_sql;

namespace {

void register_bound_create(Session &session, const BoundStatement &bound) {
    ASSERT_EQ(bound.kind, BoundStatement::Kind::CreateTable);
    ASSERT_TRUE(session.current_database_catalog()
                    .create_table(bound.create_table.schema_name, bound.create_table.schema)
                    .has_value());
}

} // namespace

TEST(BinderTest, BindsCreateInsertAndSelect) {
    Cluster cluster;
    Session session(cluster);

    const auto create = bind_sql(session, "CREATE TABLE users (id INT, name TEXT);");
    ASSERT_TRUE(create.has_value());
    register_bound_create(session, create.value());

    const auto insert = bind_sql(session, "INSERT INTO users VALUES (1, 'Alice');");
    ASSERT_TRUE(insert.has_value());
    EXPECT_EQ(insert.value().kind, BoundStatement::Kind::Insert);
    EXPECT_EQ(insert.value().insert.schema_name, "public");
    EXPECT_EQ(insert.value().insert.table_name, "users");
    ASSERT_EQ(insert.value().insert.values.size(), 2U);
    EXPECT_EQ(insert.value().insert.values[0].as_int(), 1);
    EXPECT_EQ(insert.value().insert.values[1].as_text(), "Alice");

    const auto select = bind_sql(session, "SELECT name FROM users WHERE id > 0;");
    ASSERT_TRUE(select.has_value());
    EXPECT_EQ(select.value().kind, BoundStatement::Kind::Select);
    EXPECT_EQ(select.value().select.schema_name, "public");
    EXPECT_EQ(select.value().select.table_name, "users");
    ASSERT_EQ(select.value().select.column_ordinals.size(), 1U);
    EXPECT_EQ(select.value().select.column_ordinals.front(), 1U);
    ASSERT_NE(select.value().select.where, nullptr);
    EXPECT_EQ(select.value().select.where->kind, BoundExpressionKind::Comparison);
}

TEST(BinderTest, BindsQualifiedTableNames) {
    Cluster cluster;
    Session session(cluster);
    Executor executor(session);

    auto create_schema = bind_sql(session, "CREATE SCHEMA analytics;");
    ASSERT_TRUE(create_schema.has_value());
    ASSERT_TRUE(executor.execute(std::move(create_schema.value())).has_value());

    const auto create =
        bind_sql(session, "CREATE TABLE analytics.events (id INT, name TEXT);");
    ASSERT_TRUE(create.has_value());
    EXPECT_EQ(create.value().create_table.schema_name, "analytics");
    register_bound_create(session, create.value());

    const auto select = bind_sql(session, "SELECT name FROM analytics.events WHERE id > 0;");
    ASSERT_TRUE(select.has_value());
    EXPECT_EQ(select.value().select.schema_name, "analytics");
    EXPECT_EQ(select.value().select.table_name, "events");
}

TEST(BinderTest, RejectsUnknownTable) {
    Cluster cluster;
    Session session(cluster);
    EXPECT_FALSE(bind_sql(session, "SELECT * FROM users;").has_value());
}

TEST(BinderTest, RejectsUnknownColumn) {
    Cluster cluster;
    Session session(cluster);

    const auto create = bind_sql(session, "CREATE TABLE users (id INT);");
    ASSERT_TRUE(create.has_value());
    register_bound_create(session, create.value());

    EXPECT_FALSE(bind_sql(session, "SELECT missing FROM users;").has_value());
}

TEST(BinderTest, RejectsInvalidComparisonTypes) {
    Cluster cluster;
    Session session(cluster);

    const auto create = bind_sql(session, "CREATE TABLE users (id INT, name TEXT);");
    ASSERT_TRUE(create.has_value());
    register_bound_create(session, create.value());

    EXPECT_FALSE(bind_sql(session, "SELECT name FROM users WHERE name > 0;").has_value());
}

TEST(BinderTest, RejectsLiteralWhereClause) {
    Cluster cluster;
    Session session(cluster);

    const auto create = bind_sql(session, "CREATE TABLE users (id INT);");
    ASSERT_TRUE(create.has_value());
    register_bound_create(session, create.value());

    EXPECT_FALSE(bind_sql(session, "SELECT id FROM users WHERE 1;").has_value());
}

TEST(BinderTest, RejectsBareColumnWhereClause) {
    Cluster cluster;
    Session session(cluster);

    const auto create = bind_sql(session, "CREATE TABLE users (id INT);");
    ASSERT_TRUE(create.has_value());
    register_bound_create(session, create.value());

    EXPECT_FALSE(bind_sql(session, "SELECT id FROM users WHERE id;").has_value());
}
