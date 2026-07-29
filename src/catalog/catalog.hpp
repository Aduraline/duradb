#pragma once

#include "catalog/schema.hpp"
#include "catalog/value.hpp"
#include "common/result.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace duradb {

class Catalog {
  public:
    Status create_table(TableSchema schema);

    const TableSchema *find_table(std::string_view name) const;
    bool table_exists(std::string_view name) const;

    Status validate_insert(std::string_view table_name, const std::vector<Value> &values) const;

  private:
    std::unordered_map<std::string, TableSchema> tables_; // TODO: persist to storage pages
};

} // namespace duradb
