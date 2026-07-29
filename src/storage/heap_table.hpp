#pragma once

#include "catalog/schema.hpp"
#include "common/result.hpp"
#include "storage/row.hpp"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace duradb {

class HeapTable {
  public:
    Status create_table(TableSchema schema);

    Status insert(std::string_view table_name, Row row);

    Result<std::vector<Row>> scan(std::string_view table_name) const;

    bool table_exists(std::string_view table_name) const;

  private:
    struct TableStorage {
        TableSchema schema;
        std::vector<Row> rows; // TODO: slotted pages via buffer pool
    };

    std::unordered_map<std::string, TableStorage> tables_; // TODO: durable heap file on disk
};

} // namespace duradb
