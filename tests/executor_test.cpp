#include "execution/executor.hpp"

#include "support/engine_test_utils.hpp"

#include <gtest/gtest.h>

using duradb::ExecutionResult;
using duradb::Executor;
using duradb::DatabaseEngine;
using duradb::test::bind_sql;

TEST(ExecutorTest, ExecutesCreateInsertAndSelect) {
    DatabaseEngine engine;
    Executor executor(engine);

    auto create = bind_sql(engine, "CREATE TABLE users (id INT, name TEXT);");
    ASSERT_TRUE(create.has_value());
    ASSERT_TRUE(executor.execute(std::move(create.value())).has_value());

    auto insert = bind_sql(engine, "INSERT INTO users VALUES (1, 'Ada');");
    ASSERT_TRUE(insert.has_value());
    ASSERT_TRUE(executor.execute(std::move(insert.value())).has_value());

    auto select = bind_sql(engine, "SELECT name FROM users WHERE id > 0;");
    ASSERT_TRUE(select.has_value());
    const auto result = executor.execute(std::move(select.value()));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().kind, ExecutionResult::Kind::Rows);
    ASSERT_EQ(result.value().rows.size(), 1U);
    ASSERT_EQ(result.value().rows.front().size(), 1U);
    EXPECT_EQ(result.value().rows.front().front().as_text(), "Ada");
}

TEST(ExecutorTest, FiltersRowsWithWhereClause) {
    DatabaseEngine engine;
    Executor executor(engine);

    auto create = bind_sql(engine, "CREATE TABLE users (id INT, name TEXT);");
    ASSERT_TRUE(create.has_value());
    ASSERT_TRUE(executor.execute(std::move(create.value())).has_value());

    auto first_insert = bind_sql(engine, "INSERT INTO users VALUES (1, 'Ada');");
    ASSERT_TRUE(first_insert.has_value());
    ASSERT_TRUE(executor.execute(std::move(first_insert.value())).has_value());

    auto second_insert = bind_sql(engine, "INSERT INTO users VALUES (2, 'Bob');");
    ASSERT_TRUE(second_insert.has_value());
    ASSERT_TRUE(executor.execute(std::move(second_insert.value())).has_value());

    auto select = bind_sql(engine, "SELECT name FROM users WHERE id > 1;");
    ASSERT_TRUE(select.has_value());
    const auto result = executor.execute(std::move(select.value()));
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().rows.size(), 1U);
    EXPECT_EQ(result.value().rows.front().front().as_text(), "Bob");
}
