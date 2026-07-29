#pragma once

#include "catalog/value.hpp"

#include <string>
#include <vector>

namespace duradb {

struct ExecutionResult {
    enum class Kind { Ok, Rows } kind{Kind::Ok};

    std::vector<std::vector<Value>> rows;
    std::vector<std::string> column_names;
};

} // namespace duradb
