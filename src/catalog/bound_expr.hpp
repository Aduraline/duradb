#pragma once

#include "catalog/schema.hpp"
#include "catalog/value.hpp"
#include "common/result.hpp"
#include "frontend/ast.hpp"
#include "storage/row.hpp"

#include <cstddef>
#include <memory>

namespace duradb {

enum class BoundComparisonOperator {
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
};

enum class BoundExpressionKind {
    Literal,
    ColumnRef,
    Comparison,
    And,
    Or,
};

struct BoundExpression {
    BoundExpressionKind kind{BoundExpressionKind::Literal};

    Value literal;
    std::size_t column_ordinal{};
    LogicalType column_type{LogicalType::Int};
    BoundComparisonOperator comparison_op{BoundComparisonOperator::Equal};
    std::unique_ptr<BoundExpression> left;
    std::unique_ptr<BoundExpression> right;
};

Result<std::unique_ptr<BoundExpression>> bind_expression(const Expression &expression,
                                                         const TableSchema &schema);

Status validate_bound_expression_ordinals(const BoundExpression &expression,
                                          std::size_t column_count);

Status validate_bound_predicate(const BoundExpression &expression);

Result<bool> evaluate(const BoundExpression &expression, const Row &row);

} // namespace duradb
