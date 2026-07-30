#include "engine/session.hpp"

#include "catalog/catalog_constants.hpp"

namespace duradb {

Session::Session(Cluster &cluster, std::string database_name)
    : cluster_(&cluster), current_database_(std::move(database_name)) {
    if (!cluster.database_exists(current_database_)) {
        current_database_ = std::string(kDefaultDatabase);
    }
}

Cluster &Session::cluster() { return *cluster_; }

const Cluster &Session::cluster() const { return *cluster_; }

std::string_view Session::current_database() const { return current_database_; }

Status Session::connect(std::string_view database_name) {
    if (!cluster_->database_exists(database_name)) {
        return Status::fail(Error{"database not found"});
    }

    current_database_ = std::string(database_name);
    return Status::ok(Unit{});
}

DatabaseCatalog &Session::current_database_catalog() {
    DatabaseCatalog *database = cluster_->find_database(current_database_);
    return *database;
}

const DatabaseCatalog &Session::current_database_catalog() const {
    const DatabaseCatalog *database = cluster_->find_database(current_database_);
    return *database;
}

} // namespace duradb
