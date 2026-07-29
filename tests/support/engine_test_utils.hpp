#pragma once

#include "catalog/binder.hpp"
#include "engine/database_engine.hpp"
#include "frontend/parser.hpp"

namespace duradb::test {

inline Result<BoundStatement> bind_sql(DatabaseEngine &engine, std::string_view sql) {
    Parser parser(sql);
    ParseResult<Statement> parsed = parser.parse_statement();
    if (!parsed.has_value()) {
        return Result<BoundStatement>::fail(Error{parsed.error().message});
    }

    Binder binder(engine);
    return binder.bind(std::move(parsed.value()));
}

} // namespace duradb::test
