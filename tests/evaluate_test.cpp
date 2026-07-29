#include "catalog/bound_expr.hpp"

#include <gtest/gtest.h>

using duradb::BoundComparisonOperator;
using duradb::BoundExpression;
using duradb::BoundExpressionKind;
using duradb::LogicalType;
using duradb::Row;
using duradb::Value;

namespace {

BoundExpression make_int_literal(std::int64_t value) {
    BoundExpression expression;
    expression.kind = BoundExpressionKind::Literal;
    expression.literal = Value::from_int(value);
    return expression;
}

BoundExpression make_text_literal(std::string text) {
    BoundExpression expression;
    expression.kind = BoundExpressionKind::Literal;
    expression.literal = Value::from_text(std::move(text));
    return expression;
}

BoundExpression make_column_ref(std::size_t ordinal, LogicalType type) {
    BoundExpression expression;
    expression.kind = BoundExpressionKind::ColumnRef;
    expression.column_ordinal = ordinal;
    expression.column_type = type;
    return expression;
}

BoundExpression make_comparison(BoundComparisonOperator op, BoundExpression left,
                                BoundExpression right) {
    BoundExpression expression;
    expression.kind = BoundExpressionKind::Comparison;
    expression.comparison_op = op;
    expression.left = std::make_unique<BoundExpression>(std::move(left));
    expression.right = std::make_unique<BoundExpression>(std::move(right));
    return expression;
}

} // namespace

TEST(EvaluateTest, ComparesIntegerColumnAgainstLiteral) {
    Row row{{Value::from_int(7), Value::from_text("Ada")}};

    BoundExpression predicate = make_comparison(
        BoundComparisonOperator::Greater, make_column_ref(0, LogicalType::Int), make_int_literal(0));

    const auto result = evaluate(predicate, row);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value());
}

TEST(EvaluateTest, EvaluatesAndExpression) {
    Row row{{Value::from_int(1), Value::from_text("Ada")}};

    BoundExpression predicate;
    predicate.kind = BoundExpressionKind::And;
    predicate.left = std::make_unique<BoundExpression>(make_comparison(
        BoundComparisonOperator::Greater, make_column_ref(0, LogicalType::Int), make_int_literal(0)));
    predicate.right = std::make_unique<BoundExpression>(make_comparison(
        BoundComparisonOperator::Equal, make_column_ref(1, LogicalType::Text),
        make_text_literal("Ada")));

    const auto result = evaluate(predicate, row);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value());
}

TEST(EvaluateTest, RejectsRowThatFailsPredicate) {
    Row row{{Value::from_int(0), Value::from_text("Ada")}};

    BoundExpression predicate = make_comparison(
        BoundComparisonOperator::Greater, make_column_ref(0, LogicalType::Int), make_int_literal(0));

    const auto result = evaluate(predicate, row);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result.value());
}

TEST(EvaluateTest, RejectsOutOfRangeColumnOrdinal) {
    Row row{{Value::from_int(1), Value::from_text("Ada")}};

    BoundExpression predicate = make_comparison(
        BoundComparisonOperator::Greater, make_column_ref(9, LogicalType::Int), make_int_literal(0));

    EXPECT_FALSE(evaluate(predicate, row).has_value());
}

TEST(EvaluateTest, ValidatesExpressionOrdinalsAgainstSchema) {
    BoundExpression predicate = make_comparison(
        BoundComparisonOperator::Greater, make_column_ref(2, LogicalType::Int), make_int_literal(0));

    EXPECT_FALSE(validate_bound_expression_ordinals(predicate, 2).has_value());
}
