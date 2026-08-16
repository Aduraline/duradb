#pragma once

#include "catalog/schema.hpp"
#include "catalog/value.hpp"
#include "common/result.hpp"

#include <vector>

namespace duradb {

struct Row {
    std::vector<Value> values;
};

Status validate_values(const std::vector<Value> &values, const TableSchema &schema);
Status validate_row(const Row &row, const TableSchema &schema);

} // namespace duradb
