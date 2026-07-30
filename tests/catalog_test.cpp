#include "catalog/database_catalog.hpp"
#include "engine/cluster.hpp"
#include "engine/session.hpp"

#include <gtest/gtest.h>

using duradb::Cluster;
using duradb::DatabaseCatalog;
using duradb::LogicalType;
using duradb::Session;
using duradb::TableSchema;
using duradb::kDefaultDatabase;
using duradb::kPublicSchema;

TEST(CatalogTest, BootstrapsDefaultDatabaseWithPublicSchema) {
    Cluster cluster;

    ASSERT_TRUE(cluster.database_exists(kDefaultDatabase));
    const DatabaseCatalog *database = cluster.find_database(kDefaultDatabase);
    ASSERT_NE(database, nullptr);
    EXPECT_TRUE(database->schema_exists(kPublicSchema));
}

TEST(CatalogTest, CreatesTablesInPublicSchemaByDefault) {
    DatabaseCatalog catalog;

    TableSchema schema;
    schema.name = "users";
    schema.columns = {{"id", LogicalType::Int, 0}};

    ASSERT_TRUE(catalog.create_table(kPublicSchema, std::move(schema)).has_value());
    EXPECT_TRUE(catalog.table_exists(kPublicSchema, "users"));
}

TEST(CatalogTest, IsolatesTablesAcrossSchemas) {
    DatabaseCatalog catalog;

    ASSERT_TRUE(catalog.create_schema("analytics").has_value());

    TableSchema public_schema;
    public_schema.name = "events";
    public_schema.columns = {{"id", LogicalType::Int, 0}};

    TableSchema analytics_schema;
    analytics_schema.name = "events";
    analytics_schema.columns = {{"name", LogicalType::Text, 0}};

    ASSERT_TRUE(catalog.create_table(kPublicSchema, public_schema).has_value());
    ASSERT_TRUE(catalog.create_table("analytics", std::move(analytics_schema)).has_value());

    const TableSchema *public_table = catalog.find_table(kPublicSchema, "events");
    const TableSchema *analytics_table = catalog.find_table("analytics", "events");
    ASSERT_NE(public_table, nullptr);
    ASSERT_NE(analytics_table, nullptr);
    EXPECT_EQ(public_table->columns.front().type, LogicalType::Int);
    EXPECT_EQ(analytics_table->columns.front().type, LogicalType::Text);
}

TEST(CatalogTest, IsolatesDatabasesInCluster) {
    Cluster cluster;
    Session session(cluster);

    TableSchema schema;
    schema.name = "users";
    schema.columns = {{"id", LogicalType::Int, 0}};
    ASSERT_TRUE(session.current_database_catalog().create_table(kPublicSchema, schema).has_value());

    ASSERT_TRUE(cluster.create_database("app").has_value());
    ASSERT_TRUE(session.connect("app").has_value());
    EXPECT_FALSE(session.current_database_catalog().table_exists(kPublicSchema, "users"));
}
