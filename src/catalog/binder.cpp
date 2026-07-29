#include "catalog/binder.hpp"

namespace duradb {

namespace {

bool is_literal_expression(const Expression &expression) {
    return dynamic_cast<const IntegerLiteralExpression *>(&expression) != nullptr ||
           dynamic_cast<const StringLiteralExpression *>(&expression) != nullptr;
}

} // namespace

Binder::Binder(const DatabaseEngine &engine) : engine_(engine) {}

Result<BoundStatement> Binder::bind(Statement statement) const {
    switch (statement.kind) {
    case StatementKind::CreateTable:
        return bind_create_table(statement.create_table);
    case StatementKind::Insert:
        return bind_insert(statement.insert);
    case StatementKind::Select:
        return bind_select(std::move(statement.select));
    }

    return Result<BoundStatement>::fail(Error{"unsupported statement"});
}

Result<BoundStatement> Binder::bind_create_table(const CreateTableStatement &statement) const {
    if (engine_.table_exists(statement.table)) {
        return Result<BoundStatement>::fail(Error{"table already exists"});
    }

    BoundStatement bound;
    bound.kind = BoundStatement::Kind::CreateTable;
    bound.create_table.schema = table_schema_from_ast(statement);
    return Result<BoundStatement>::ok(std::move(bound));
}

Result<BoundStatement> Binder::bind_insert(const InsertStatement &statement) const {
    const TableSchema *table = engine_.find_table(statement.table);
    if (table == nullptr) {
        return Result<BoundStatement>::fail(Error{"table not found"});
    }

    BoundInsertStatement bound_insert;
    bound_insert.table_name = std::string(statement.table);
    bound_insert.values.reserve(statement.values.size());

    for (const std::unique_ptr<Expression> &value_expression : statement.values) {
        if (!is_literal_expression(*value_expression)) {
            return Result<BoundStatement>::fail(Error{"expected literal value in insert"});
        }

        Result<Value> literal = Value::from_expression(*value_expression);
        if (!literal.has_value()) {
            return Result<BoundStatement>::fail(literal.error());
        }

        bound_insert.values.push_back(std::move(literal.value()));
    }

    if (const Status validation = engine_.validate_insert(statement.table, bound_insert.values);
        !validation.has_value()) {
        return Result<BoundStatement>::fail(validation.error());
    }

    BoundStatement bound;
    bound.kind = BoundStatement::Kind::Insert;
    bound.insert = std::move(bound_insert);
    return Result<BoundStatement>::ok(std::move(bound));
}

Result<BoundStatement> Binder::bind_select(SelectStatement statement) const {
    const TableSchema *table = engine_.find_table(statement.table);
    if (table == nullptr) {
        return Result<BoundStatement>::fail(Error{"table not found"});
    }

    BoundSelectStatement bound_select;
    bound_select.table_name = std::string(statement.table);
    bound_select.select_all = statement.select_all;

    if (statement.where != nullptr) {
        Result<std::unique_ptr<BoundExpression>> bound_where =
            bind_expression(*statement.where, *table);
        if (!bound_where.has_value()) {
            return Result<BoundStatement>::fail(bound_where.error());
        }

        if (const Status validation = validate_bound_predicate(*bound_where.value());
            !validation.has_value()) {
            return Result<BoundStatement>::fail(validation.error());
        }

        bound_select.where = std::move(bound_where.value());
    }

    if (statement.select_all) {
        bound_select.column_ordinals.reserve(table->columns.size());
        for (const ColumnSchema &column : table->columns) {
            bound_select.column_ordinals.push_back(column.ordinal);
        }
    } else {
        bound_select.column_ordinals.reserve(statement.columns.size());

        for (const std::unique_ptr<Expression> &column_expression : statement.columns) {
            const auto *column_ref =
                dynamic_cast<const ColumnRefExpression *>(column_expression.get());
            if (column_ref == nullptr) {
                return Result<BoundStatement>::fail(Error{"expected column reference"});
            }

            const std::optional<std::size_t> ordinal = table->column_index(column_ref->name);
            if (!ordinal.has_value()) {
                return Result<BoundStatement>::fail(Error{"column not found"});
            }

            bound_select.column_ordinals.push_back(*ordinal);
        }
    }

    BoundStatement bound;
    bound.kind = BoundStatement::Kind::Select;
    bound.select = std::move(bound_select);
    return Result<BoundStatement>::ok(std::move(bound));
}

} // namespace duradb
