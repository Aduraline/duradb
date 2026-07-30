#include "execution/executor.hpp"

#include "catalog/bound_expr.hpp"
#include "storage/row.hpp"

#include <optional>

namespace duradb {

Executor::Executor(Session &session) : session_(session) {}

Result<ExecutionResult> Executor::execute(BoundStatement bound) {
    switch (bound.kind) {
    case BoundStatement::Kind::CreateTable:
        return execute_create_table(std::move(bound.create_table));
    case BoundStatement::Kind::CreateSchema:
        return execute_create_schema(std::move(bound.create_schema));
    case BoundStatement::Kind::CreateDatabase:
        return execute_create_database(std::move(bound.create_database));
    case BoundStatement::Kind::Insert:
        return execute_insert(std::move(bound.insert));
    case BoundStatement::Kind::Select:
        return execute_select(std::move(bound.select));
    }

    return Result<ExecutionResult>::fail(Error{"unsupported statement"});
}

Result<ExecutionResult> Executor::execute_create_table(BoundCreateTableStatement bound) {
    DatabaseCatalog &catalog = session_.current_database_catalog();
    if (const Status status =
            catalog.create_table(bound.schema_name, std::move(bound.schema)); !status.has_value()) {
        return Result<ExecutionResult>::fail(status.error());
    }

    ExecutionResult result;
    result.kind = ExecutionResult::Kind::Ok;
    return Result<ExecutionResult>::ok(std::move(result));
}

Result<ExecutionResult> Executor::execute_create_schema(BoundCreateSchemaStatement bound) {
    DatabaseCatalog &catalog = session_.current_database_catalog();
    if (const Status status = catalog.create_schema(std::move(bound.schema_name));
        !status.has_value()) {
        return Result<ExecutionResult>::fail(status.error());
    }

    ExecutionResult result;
    result.kind = ExecutionResult::Kind::Ok;
    return Result<ExecutionResult>::ok(std::move(result));
}

Result<ExecutionResult> Executor::execute_create_database(BoundCreateDatabaseStatement bound) {
    if (const Status status = session_.cluster().create_database(std::move(bound.database_name));
        !status.has_value()) {
        return Result<ExecutionResult>::fail(status.error());
    }

    ExecutionResult result;
    result.kind = ExecutionResult::Kind::Ok;
    return Result<ExecutionResult>::ok(std::move(result));
}

Result<ExecutionResult> Executor::execute_insert(BoundInsertStatement bound) {
    DatabaseCatalog &catalog = session_.current_database_catalog();
    if (catalog.find_table(bound.schema_name, bound.table_name) == nullptr) {
        return Result<ExecutionResult>::fail(Error{"table not found"});
    }

    if (const Status status = catalog.insert(bound.schema_name, bound.table_name,
                                             Row{std::move(bound.values)}); !status.has_value()) {
        return Result<ExecutionResult>::fail(status.error());
    }

    ExecutionResult result;
    result.kind = ExecutionResult::Kind::Ok;
    return Result<ExecutionResult>::ok(std::move(result));
}

Result<ExecutionResult> Executor::execute_select(BoundSelectStatement bound) {
    const DatabaseCatalog &catalog = session_.current_database_catalog();
    const TableSchema *table = catalog.find_table(bound.schema_name, bound.table_name);
    if (table == nullptr) {
        return Result<ExecutionResult>::fail(Error{"table not found"});
    }

    const std::size_t column_count = table->columns.size();

    for (const std::size_t ordinal : bound.column_ordinals) {
        if (ordinal >= column_count) {
            return Result<ExecutionResult>::fail(Error{"column ordinal out of range"});
        }
    }

    if (bound.where != nullptr) {
        if (const Status validation = validate_bound_predicate(*bound.where); !validation.has_value()) {
            return Result<ExecutionResult>::fail(validation.error());
        }

        if (const Status validation =
                validate_bound_expression_ordinals(*bound.where, column_count);
            !validation.has_value()) {
            return Result<ExecutionResult>::fail(validation.error());
        }
    }

    ExecutionResult result;
    result.kind = ExecutionResult::Kind::Rows;
    result.column_names.reserve(bound.column_ordinals.size());

    for (const std::size_t ordinal : bound.column_ordinals) {
        result.column_names.push_back(table->columns[ordinal].name);
    }

    std::optional<Error> evaluation_error;

    const Status scan_status =
        catalog.for_each_row(bound.schema_name, bound.table_name, [&](const Row &row) {
            if (evaluation_error.has_value()) {
                return;
            }

            if (bound.where != nullptr) {
                Result<bool> matches = evaluate(*bound.where, row);
                if (!matches.has_value()) {
                    evaluation_error = matches.error();
                    return;
                }

                if (!matches.value()) {
                    return;
                }
            }

            std::vector<Value> projected;
            projected.reserve(bound.column_ordinals.size());

            for (const std::size_t ordinal : bound.column_ordinals) {
                projected.push_back(row.values[ordinal]);
            }

            result.rows.push_back(std::move(projected));
        });

    if (evaluation_error.has_value()) {
        return Result<ExecutionResult>::fail(*evaluation_error);
    }

    if (!scan_status.has_value()) {
        return Result<ExecutionResult>::fail(scan_status.error());
    }

    return Result<ExecutionResult>::ok(std::move(result));
}

} // namespace duradb
