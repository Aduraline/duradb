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

Status validate_bound_expression_ordinals_impl(const BoundExpression &expression,
                                               std::size_t column_count) {
    if (expression.kind == BoundExpressionKind::ColumnRef) {
        if (expression.column_ordinal >= column_count) {
            return Status::fail(Error{"column ordinal out of range"});
        }

        return Status::ok(Unit{});
    }

    if (expression.left != nullptr) {
        if (const Status left =
                validate_bound_expression_ordinals_impl(*expression.left, column_count);
            !left.has_value()) {
            return left;
        }
    }

    if (expression.right != nullptr) {
        if (const Status right =
                validate_bound_expression_ordinals_impl(*expression.right, column_count);
            !right.has_value()) {
            return right;
        }
    }

    return Status::ok(Unit{});
}

Result<Value> evaluate_value(const BoundExpression &expression, const Row &row) {
    switch (expression.kind) {
    case BoundExpressionKind::Literal:
        return Result<Value>::ok(expression.literal);
    case BoundExpressionKind::ColumnRef:
        if (expression.column_ordinal >= row.values.size()) {
            return Result<Value>::fail(Error{"column ordinal out of range"});
        }

        return Result<Value>::ok(row.values[expression.column_ordinal]);
    case BoundExpressionKind::Comparison:
    case BoundExpressionKind::And:
    case BoundExpressionKind::Or:
        break;
    }

    return Result<Value>::fail(Error{"unsupported value expression"});
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

Status validate_bound_expression_ordinals(const BoundExpression &expression,
                                          std::size_t column_count) {
    return validate_bound_expression_ordinals_impl(expression, column_count);
}

Result<bool> evaluate(const BoundExpression &expression, const Row &row) {
    switch (expression.kind) {
    case BoundExpressionKind::Comparison: {
        Result<Value> left = evaluate_value(*expression.left, row);
        if (!left.has_value()) {
            return Result<bool>::fail(left.error());
        }

        Result<Value> right = evaluate_value(*expression.right, row);
        if (!right.has_value()) {
            return Result<bool>::fail(right.error());
        }

        return Result<bool>::ok(
            compare_values(expression.comparison_op, left.value(), right.value()));
    }
    case BoundExpressionKind::And: {
        Result<bool> left = evaluate(*expression.left, row);
        if (!left.has_value()) {
            return left;
        }

        if (!left.value()) {
            return Result<bool>::ok(false);
        }

        return evaluate(*expression.right, row);
    }
    case BoundExpressionKind::Or: {
        Result<bool> left = evaluate(*expression.left, row);
        if (!left.has_value()) {
            return left;
        }

        if (left.value()) {
            return Result<bool>::ok(true);
        }

        return evaluate(*expression.right, row);
    }
    case BoundExpressionKind::Literal:
    case BoundExpressionKind::ColumnRef:
        break;
    }

    return Result<bool>::fail(Error{"unsupported predicate expression"});
}

} // namespace duradb
