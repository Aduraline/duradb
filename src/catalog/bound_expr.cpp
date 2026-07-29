#include "catalog/bound_expr.hpp"

namespace duradb {

namespace {

BoundComparisonOperator to_bound_comparison(BinaryOperator op) {
    switch (op) {
    case BinaryOperator::Equal:
        return BoundComparisonOperator::Equal;
    case BinaryOperator::NotEqual:
        return BoundComparisonOperator::NotEqual;
    case BinaryOperator::Less:
        return BoundComparisonOperator::Less;
    case BinaryOperator::LessEqual:
        return BoundComparisonOperator::LessEqual;
    case BinaryOperator::Greater:
        return BoundComparisonOperator::Greater;
    case BinaryOperator::GreaterEqual:
        return BoundComparisonOperator::GreaterEqual;
    case BinaryOperator::And:
    case BinaryOperator::Or:
        break;
    }

    return BoundComparisonOperator::Equal;
}

LogicalType expression_type(const BoundExpression &expression) {
    switch (expression.kind) {
    case BoundExpressionKind::Literal:
        return expression.literal.type;
    case BoundExpressionKind::ColumnRef:
        return expression.column_type;
    case BoundExpressionKind::Comparison:
    case BoundExpressionKind::And:
    case BoundExpressionKind::Or:
        return LogicalType::Int;
    }

    return LogicalType::Int;
}

bool is_ordering_operator(BoundComparisonOperator op) {
    return op == BoundComparisonOperator::Less || op == BoundComparisonOperator::LessEqual ||
           op == BoundComparisonOperator::Greater || op == BoundComparisonOperator::GreaterEqual;
}

Status validate_comparison(const BoundExpression &left, const BoundExpression &right,
                           BoundComparisonOperator op) {
    const LogicalType left_type = expression_type(left);
    const LogicalType right_type = expression_type(right);

    if (left_type != right_type) {
        return Status::fail(Error{"comparison type mismatch"});
    }

    if (is_ordering_operator(op) && left_type != LogicalType::Int) {
        return Status::fail(Error{"ordering comparison requires integer operands"});
    }

    return Status::ok(Unit{});
}

Result<std::unique_ptr<BoundExpression>> bind_expression_impl(const Expression &expression,
                                                              const TableSchema &schema) {
    if (const auto *integer = dynamic_cast<const IntegerLiteralExpression *>(&expression)) {
        auto bound = std::make_unique<BoundExpression>();
        bound->kind = BoundExpressionKind::Literal;
        bound->literal = Value::from_int(integer->value);
        return Result<std::unique_ptr<BoundExpression>>::ok(std::move(bound));
    }

    if (dynamic_cast<const StringLiteralExpression *>(&expression) != nullptr) {
        Result<Value> literal = Value::from_expression(expression);
        if (!literal.has_value()) {
            return Result<std::unique_ptr<BoundExpression>>::fail(literal.error());
        }

        auto bound = std::make_unique<BoundExpression>();
        bound->kind = BoundExpressionKind::Literal;
        bound->literal = std::move(literal.value());
        return Result<std::unique_ptr<BoundExpression>>::ok(std::move(bound));
    }

    if (const auto *column_ref = dynamic_cast<const ColumnRefExpression *>(&expression)) {
        const std::optional<std::size_t> ordinal = schema.column_index(column_ref->name);
        if (!ordinal.has_value()) {
            return Result<std::unique_ptr<BoundExpression>>::fail(Error{"column not found"});
        }

        auto bound = std::make_unique<BoundExpression>();
        bound->kind = BoundExpressionKind::ColumnRef;
        bound->column_ordinal = *ordinal;
        bound->column_type = schema.columns[*ordinal].type;
        return Result<std::unique_ptr<BoundExpression>>::ok(std::move(bound));
    }

    const auto *binary = dynamic_cast<const BinaryExpression *>(&expression);
    if (binary == nullptr) {
        return Result<std::unique_ptr<BoundExpression>>::fail(Error{"unsupported expression"});
    }

    if (binary->op == BinaryOperator::And || binary->op == BinaryOperator::Or) {
        Result<std::unique_ptr<BoundExpression>> left = bind_expression_impl(*binary->left, schema);
        if (!left.has_value()) {
            return left;
        }

        Result<std::unique_ptr<BoundExpression>> right =
            bind_expression_impl(*binary->right, schema);
        if (!right.has_value()) {
            return right;
        }

        auto bound = std::make_unique<BoundExpression>();
        bound->kind =
            binary->op == BinaryOperator::And ? BoundExpressionKind::And : BoundExpressionKind::Or;
        bound->left = std::move(left.value());
        bound->right = std::move(right.value());
        return Result<std::unique_ptr<BoundExpression>>::ok(std::move(bound));
    }

    Result<std::unique_ptr<BoundExpression>> left = bind_expression_impl(*binary->left, schema);
    if (!left.has_value()) {
        return left;
    }

    Result<std::unique_ptr<BoundExpression>> right = bind_expression_impl(*binary->right, schema);
    if (!right.has_value()) {
        return right;
    }

    if (const Status validation =
            validate_comparison(*left.value(), *right.value(), to_bound_comparison(binary->op));
        !validation.has_value()) {
        return Result<std::unique_ptr<BoundExpression>>::fail(validation.error());
    }

    auto bound = std::make_unique<BoundExpression>();
    bound->kind = BoundExpressionKind::Comparison;
    bound->comparison_op = to_bound_comparison(binary->op);
    bound->left = std::move(left.value());
    bound->right = std::move(right.value());
    return Result<std::unique_ptr<BoundExpression>>::ok(std::move(bound));
}

Value evaluate_value(const BoundExpression &expression, const Row &row) {
    switch (expression.kind) {
    case BoundExpressionKind::Literal:
        return expression.literal;
    case BoundExpressionKind::ColumnRef:
        return row.values[expression.column_ordinal];
    case BoundExpressionKind::Comparison:
    case BoundExpressionKind::And:
    case BoundExpressionKind::Or:
        break;
    }

    return Value::from_int(0);
}

bool compare_values(BoundComparisonOperator op, const Value &left, const Value &right) {
    if (left.type != right.type) {
        return false;
    }

    switch (op) {
    case BoundComparisonOperator::Equal:
        if (left.type == LogicalType::Int) {
            return left.as_int() == right.as_int();
        }
        return left.as_text() == right.as_text();
    case BoundComparisonOperator::NotEqual:
        if (left.type == LogicalType::Int) {
            return left.as_int() != right.as_int();
        }
        return left.as_text() != right.as_text();
    case BoundComparisonOperator::Less:
        return left.as_int() < right.as_int();
    case BoundComparisonOperator::LessEqual:
        return left.as_int() <= right.as_int();
    case BoundComparisonOperator::Greater:
        return left.as_int() > right.as_int();
    case BoundComparisonOperator::GreaterEqual:
        return left.as_int() >= right.as_int();
    }

    return false;
}

} // namespace

Result<std::unique_ptr<BoundExpression>> bind_expression(const Expression &expression,
                                                         const TableSchema &schema) {
    return bind_expression_impl(expression, schema);
}

bool evaluate(const BoundExpression &expression, const Row &row) {
    switch (expression.kind) {
    case BoundExpressionKind::Comparison:
        return compare_values(expression.comparison_op,
                              evaluate_value(*expression.left, row),
                              evaluate_value(*expression.right, row));
    case BoundExpressionKind::And:
        return evaluate(*expression.left, row) && evaluate(*expression.right, row);
    case BoundExpressionKind::Or:
        return evaluate(*expression.left, row) || evaluate(*expression.right, row);
    case BoundExpressionKind::Literal:
    case BoundExpressionKind::ColumnRef:
        break;
    }

    return false;
}

} // namespace duradb
