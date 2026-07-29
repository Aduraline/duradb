#include "execution/executor.hpp"

#include "catalog/bound_expr.hpp"
#include "storage/row.hpp"

namespace duradb {

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
    if (const Status status =
            engine_.insert(bound.table->name, Row{std::move(bound.values)}); !status.has_value()) {
        return Result<ExecutionResult>::fail(status.error());
    }

    ExecutionResult result;
    result.kind = ExecutionResult::Kind::Ok;
    return Result<ExecutionResult>::ok(std::move(result));
}

Result<ExecutionResult> Executor::execute_select(BoundSelectStatement bound) {
    ExecutionResult result;
    result.kind = ExecutionResult::Kind::Rows;
    result.column_names.reserve(bound.column_ordinals.size());

    for (const std::size_t ordinal : bound.column_ordinals) {
        result.column_names.push_back(bound.table->columns[ordinal].name);
    }

    const Status scan_status = engine_.for_each_row(bound.table->name, [&](const Row &row) {
        if (bound.where != nullptr && !evaluate(*bound.where, row)) {
            return;
        }

        std::vector<Value> projected;
        projected.reserve(bound.column_ordinals.size());

        for (const std::size_t ordinal : bound.column_ordinals) {
            projected.push_back(row.values[ordinal]);
        }

        result.rows.push_back(std::move(projected));
    });

    if (!scan_status.has_value()) {
        return Result<ExecutionResult>::fail(scan_status.error());
    }

    return Result<ExecutionResult>::ok(std::move(result));
}

} // namespace duradb
