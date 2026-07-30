#pragma once

#include "catalog/binder.hpp"
#include "common/result.hpp"
#include "engine/session.hpp"
#include "execution/execution_result.hpp"

namespace duradb {

class Executor {
  public:
    explicit Executor(Session &session);

    Result<ExecutionResult> execute(BoundStatement bound);

  private:
    Session &session_;

    Result<ExecutionResult> execute_create_table(BoundCreateTableStatement bound);
    Result<ExecutionResult> execute_create_schema(BoundCreateSchemaStatement bound);
    Result<ExecutionResult> execute_create_database(BoundCreateDatabaseStatement bound);
    Result<ExecutionResult> execute_insert(BoundInsertStatement bound);
    Result<ExecutionResult> execute_select(BoundSelectStatement bound);
};

} // namespace duradb
