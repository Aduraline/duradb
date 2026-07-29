#pragma once

#include "catalog/binder.hpp"
#include "common/result.hpp"
#include "engine/database_engine.hpp"
#include "execution/execution_result.hpp"

namespace duradb {

class Executor {
  public:
    explicit Executor(DatabaseEngine &engine);

    Result<ExecutionResult> execute(BoundStatement bound);

  private:
    DatabaseEngine &engine_;

    Result<ExecutionResult> execute_create(BoundCreateTableStatement bound);
    Result<ExecutionResult> execute_insert(BoundInsertStatement bound);
    Result<ExecutionResult> execute_select(BoundSelectStatement bound);
};

} // namespace duradb
