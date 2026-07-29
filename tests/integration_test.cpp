#include "support/engine_test_utils.hpp"

#include <gtest/gtest.h>

using duradb::BoundStatement;
using duradb::DatabaseEngine;
using duradb::Row;
using duradb::test::bind_sql;

TEST(IntegrationTest, BindsCreateInsertAndReadsBackRows) {
    DatabaseEngine engine;

    const auto create = bind_sql(engine, "CREATE TABLE users (id INT, name TEXT);");
    ASSERT_TRUE(create.has_value());
    ASSERT_EQ(create.value().kind, BoundStatement::Kind::CreateTable);
    ASSERT_TRUE(engine.create_table(create.value().create_table.schema).has_value());

    const auto insert = bind_sql(engine, "INSERT INTO users VALUES (1, 'Ada');");
    ASSERT_TRUE(insert.has_value());
    ASSERT_EQ(insert.value().kind, BoundStatement::Kind::Insert);
    ASSERT_TRUE(engine.insert(insert.value().insert.table->name, Row{insert.value().insert.values})
                    .has_value());

    std::size_t row_count = 0;
    ASSERT_TRUE(engine
                    .for_each_row("users",
                                  [&](const Row &row) {
                                      ++row_count;
                                      ASSERT_EQ(row.values.size(), 2U);
                                      EXPECT_EQ(row.values[0].as_int(), 1);
                                      EXPECT_EQ(row.values[1].as_text(), "Ada");
                                  })
                    .has_value());
    EXPECT_EQ(row_count, 1U);
}
