#include "storage/row.hpp"

namespace duradb {

Status validate_row(const Row &row, const TableSchema &schema) {
    if (row.values.size() != schema.columns.size()) {
        return Status::fail(Error{"row column count mismatch"});
    }

    for (std::size_t index = 0; index < row.values.size(); ++index) {
        if (row.values[index].type != schema.columns[index].type) {
            return Status::fail(Error{"row type mismatch"});
        }
    }

    return Status::ok(Unit{});
}

} // namespace duradb
