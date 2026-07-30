#pragma once

#include "catalog/catalog_constants.hpp"
#include "catalog/database_catalog.hpp"
#include "common/string_view_hash.hpp"

#include <string>
#include <string_view>

namespace duradb {

class Cluster {
  public:
    Cluster();

    Status create_database(std::string name);

    bool database_exists(std::string_view database_name) const;
    DatabaseCatalog *find_database(std::string_view database_name);
    const DatabaseCatalog *find_database(std::string_view database_name) const;

  private:
    std::unordered_map<std::string, DatabaseCatalog, StringViewHash, StringViewEqual> databases_;
};

} // namespace duradb
