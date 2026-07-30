#include "catalog/binder.hpp"

#include "catalog/table_reference.hpp"

namespace duradb {

namespace {

bool is_literal_expression(const Expression &expression) {
    return dynamic_cast<const IntegerLiteralExpression *>(&expression) != nullptr ||
           dynamic_cast<const StringLiteralExpression *>(&expression) != nullptr;
}

} // namespace

Binder::Binder(const Session &session) : session_(session) {}

Result<BoundStatement> Binder::bind(Statement statement) const {
    switch (statement.kind) {
    case StatementKind::CreateTable:
        return bind_create_table(statement.create_table);
    case StatementKind::CreateSchema:
        return bind_create_schema(statement.create_schema);
    case StatementKind::CreateDatabase:
        return bind_create_database(statement.create_database);
    case StatementKind::Insert:
        return bind_insert(statement.insert);
    case StatementKind::Select:
        return bind_select(std::move(statement.select));
    }

    return Result<BoundStatement>::fail(Error{"unsupported statement"});
}

Result<BoundStatement> Binder::bind_create_table(const CreateTableStatement &statement) const {
    const std::string schema_name = resolve_schema_name(statement.table.schema);
    const std::string_view table_name = statement.table.table;

    const DatabaseCatalog &catalog = session_.current_database_catalog();
    if (!catalog.schema_exists(schema_name)) {
        return Result<BoundStatement>::fail(Error{"schema not found"});
    }

    if (catalog.table_exists(schema_name, table_name)) {
        return Result<BoundStatement>::fail(Error{"table already exists"});
    }

    BoundStatement bound;
    bound.kind = BoundStatement::Kind::CreateTable;
    bound.create_table.schema_name = schema_name;
    bound.create_table.schema = table_schema_from_ast(statement);
    return Result<BoundStatement>::ok(std::move(bound));
}

Result<BoundStatement> Binder::bind_create_schema(const CreateSchemaStatement &statement) const {
    if (session_.current_database_catalog().schema_exists(statement.schema)) {
        return Result<BoundStatement>::fail(Error{"schema already exists"});
    }

    BoundStatement bound;
    bound.kind = BoundStatement::Kind::CreateSchema;
    bound.create_schema.schema_name = std::string(statement.schema);
    return Result<BoundStatement>::ok(std::move(bound));
}

Result<BoundStatement> Binder::bind_create_database(const CreateDatabaseStatement &statement) const {
    if (session_.cluster().database_exists(statement.database)) {
        return Result<BoundStatement>::fail(Error{"database already exists"});
    }

    BoundStatement bound;
    bound.kind = BoundStatement::Kind::CreateDatabase;
    bound.create_database.database_name = std::string(statement.database);
    return Result<BoundStatement>::ok(std::move(bound));
}

Result<BoundStatement> Binder::bind_insert(const InsertStatement &statement) const {
    const std::string schema_name = resolve_schema_name(statement.table.schema);
    const std::string_view table_name = statement.table.table;

    const DatabaseCatalog &catalog = session_.current_database_catalog();
    const TableSchema *table = catalog.find_table(schema_name, table_name);
    if (table == nullptr) {
        return Result<BoundStatement>::fail(Error{"table not found"});
    }

    BoundInsertStatement bound_insert;
    bound_insert.schema_name = schema_name;
    bound_insert.table_name = std::string(table_name);
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

    if (const Status validation =
            catalog.validate_insert(schema_name, table_name, bound_insert.values);
        !validation.has_value()) {
        return Result<BoundStatement>::fail(validation.error());
    }

    BoundStatement bound;
    bound.kind = BoundStatement::Kind::Insert;
    bound.insert = std::move(bound_insert);
    return Result<BoundStatement>::ok(std::move(bound));
}

Result<BoundStatement> Binder::bind_select(SelectStatement statement) const {
    const std::string schema_name = resolve_schema_name(statement.table.schema);
    const std::string_view table_name = statement.table.table;

    const DatabaseCatalog &catalog = session_.current_database_catalog();
    const TableSchema *table = catalog.find_table(schema_name, table_name);
    if (table == nullptr) {
        return Result<BoundStatement>::fail(Error{"table not found"});
    }

    BoundSelectStatement bound_select;
    bound_select.schema_name = schema_name;
    bound_select.table_name = std::string(table_name);
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
