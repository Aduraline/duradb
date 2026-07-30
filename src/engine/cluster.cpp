#include "engine/cluster.hpp"

#include "catalog/catalog_constants.hpp"

namespace duradb {

Cluster::Cluster() { databases_.emplace(kDefaultDatabase, DatabaseCatalog{}); }

Status Cluster::create_database(std::string name) {
    if (name.empty()) {
        return Status::fail(Error{"database name must not be empty"});
    }

    if (databases_.contains(name)) {
        return Status::fail(Error{"database already exists"});
    }

    databases_.emplace(name, DatabaseCatalog{});
    return Status::ok(Unit{});
}

bool Cluster::database_exists(std::string_view database_name) const {
    return find_database(database_name) != nullptr;
}

DatabaseCatalog *Cluster::find_database(std::string_view database_name) {
    const auto iterator = databases_.find(database_name);
    if (iterator == databases_.end()) {
        return nullptr;
    }

    return &iterator->second;
}

const DatabaseCatalog *Cluster::find_database(std::string_view database_name) const {
    const auto iterator = databases_.find(database_name);
    if (iterator == databases_.end()) {
        return nullptr;
    }

    return &iterator->second;
}

} // namespace duradb
