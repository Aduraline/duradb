#pragma once

#include "catalog/binder.hpp"
#include "engine/session.hpp"
#include "frontend/parser.hpp"

namespace duradb::test {

inline Result<BoundStatement> bind_sql(Session &session, std::string_view sql) {
    Parser parser(sql);
    ParseResult<Statement> parsed = parser.parse_statement();
    if (!parsed.has_value()) {
        return Result<BoundStatement>::fail(Error{parsed.error().message});
    }

    Binder binder(session);
    return binder.bind(std::move(parsed.value()));
}

} // namespace duradb::test
