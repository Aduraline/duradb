#pragma once

#include "catalog/binder.hpp"
#include "catalog/catalog.hpp"
#include "frontend/parser.hpp"

namespace duradb::test {

inline Result<BoundStatement> bind_sql(Catalog &catalog, std::string_view sql) {
    Parser parser(sql);
    ParseResult<Statement> parsed = parser.parse_statement();
    if (!parsed.has_value()) {
        return Result<BoundStatement>::fail(Error{parsed.error().message});
    }

    Binder binder(catalog);
    return binder.bind(std::move(parsed.value()));
}

} // namespace duradb::test
