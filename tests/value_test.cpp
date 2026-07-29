#include "catalog/value.hpp"

#include <gtest/gtest.h>

TEST(ValueTest, RejectsNonLiteralExpression) {
    duradb::ColumnRefExpression expression;
    expression.name = "id";
    EXPECT_FALSE(duradb::Value::from_expression(expression).has_value());
}
