#pragma once

#include "catalog/catalog_constants.hpp"
#include "catalog/database_catalog.hpp"
#include "common/result.hpp"
#include "engine/cluster.hpp"

#include <string>
#include <string_view>

namespace duradb {

class Session {
  public:
    Session(Cluster &cluster, std::string database_name = std::string(kDefaultDatabase));

    Cluster &cluster();
    const Cluster &cluster() const;

    std::string_view current_database() const;

    Status connect(std::string_view database_name);

    DatabaseCatalog &current_database_catalog();
    const DatabaseCatalog &current_database_catalog() const;

  private:
    Cluster *cluster_;
    std::string current_database_;
};

} // namespace duradb
