#include "execution/executor.hpp"

#include "catalog/bound_expr.hpp"
#include "storage/row.hpp"

#include <optional>

namespace duradb {

namespace {

const TableSchema *resolve_table(const DatabaseEngine &engine, std::string_view table_name) {
    return engine.find_table(table_name);
}

} // namespace

Executor::Executor(DatabaseEngine &engine) : engine_(engine) {}

Result<ExecutionResult> Executor::execute(BoundStatement bound) {
    switch (bound.kind) {
    case BoundStatement::Kind::CreateTable:
        return execute_create(std::move(bound.create_table));
    case BoundStatement::Kind::Insert:
        return execute_insert(std::move(bound.insert));
    case BoundStatement::Kind::Select:
        return execute_select(std::move(bound.select));
    }

    return Result<ExecutionResult>::fail(Error{"unsupported statement"});
}

Result<ExecutionResult> Executor::execute_create(BoundCreateTableStatement bound) {
    if (const Status status = engine_.create_table(std::move(bound.schema)); !status.has_value()) {
        return Result<ExecutionResult>::fail(status.error());
    }

    ExecutionResult result;
    result.kind = ExecutionResult::Kind::Ok;
    return Result<ExecutionResult>::ok(std::move(result));
}

Result<ExecutionResult> Executor::execute_insert(BoundInsertStatement bound) {
    if (resolve_table(engine_, bound.table_name) == nullptr) {
        return Result<ExecutionResult>::fail(Error{"table not found"});
    }

    if (const Status status =
            engine_.insert(bound.table_name, Row{std::move(bound.values)}); !status.has_value()) {
        return Result<ExecutionResult>::fail(status.error());
    }

    ExecutionResult result;
    result.kind = ExecutionResult::Kind::Ok;
    return Result<ExecutionResult>::ok(std::move(result));
}

Result<ExecutionResult> Executor::execute_select(BoundSelectStatement bound) {
    const TableSchema *table = resolve_table(engine_, bound.table_name);
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

    const Status scan_status = engine_.for_each_row(bound.table_name, [&](const Row &row) {
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
