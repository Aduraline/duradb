#include "storage/row.hpp"

namespace duradb {

namespace {

bool payload_matches_type(LogicalType type, const std::variant<std::int64_t, std::string> &payload) {
    switch (type) {
    case LogicalType::Int:
        return std::holds_alternative<std::int64_t>(payload);
    case LogicalType::Text:
        return std::holds_alternative<std::string>(payload);
    }

    return false;
}

} // namespace

Status validate_values(const std::vector<Value> &values, const TableSchema &schema) {
    if (values.size() != schema.columns.size()) {
        return Status::fail(Error{"row column count mismatch"});
    }

    for (std::size_t index = 0; index < values.size(); ++index) {
        const Value &value = values[index];
        if (value.type != schema.columns[index].type) {
            return Status::fail(Error{"row type mismatch"});
        }

        if (!payload_matches_type(value.type, value.payload)) {
            return Status::fail(Error{"value payload mismatch"});
        }
    }

    return Status::ok(Unit{});
}

Status validate_row(const Row &row, const TableSchema &schema) {
    return validate_values(row.values, schema);
}

} // namespace duradb
