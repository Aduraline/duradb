#pragma once

#include "catalog/catalog_constants.hpp"

#include <string>
#include <string_view>

namespace duradb {

struct TableReference {
    std::string_view schema;
    std::string_view table;
};

inline std::string resolve_schema_name(std::string_view schema) {
    if (schema.empty()) {
        return std::string(kPublicSchema);
    }

    return std::string(schema);
}

} // namespace duradb
