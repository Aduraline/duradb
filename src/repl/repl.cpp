#include "repl/repl.hpp"

#include "catalog/binder.hpp"
#include "execution/executor.hpp"
#include "frontend/parser.hpp"

#include <iostream>
#include <string>
#include <string_view>

namespace duradb {

namespace {

bool is_exit_command(std::string_view line) {
    return line == ".quit" || line == ".exit";
}

bool is_help_command(std::string_view line) {
    return line == ".help";
}

bool is_connect_command(std::string_view line) {
    return line.starts_with(".connect ");
}

std::string_view connect_target(std::string_view line) {
    return line.substr(std::string_view(".connect ").size());
}

void print_help(std::ostream &output) {
    output << "Commands:\n";
    output << "  .help              show this message\n";
    output << "  .connect <db>      switch to another database\n";
    output << "  .quit              exit the shell\n";
    output << "SQL:\n";
    output << "  CREATE DATABASE ...;\n";
    output << "  CREATE SCHEMA ...;\n";
    output << "  CREATE TABLE ...;\n";
    output << "  INSERT INTO ... VALUES (...);\n";
    output << "  SELECT ... FROM ... [WHERE ...];\n";
}

void print_parse_error(const ParseError &error, std::ostream &output) {
    output << "parse error at " << error.line << ':' << error.column << ": " << error.message
           << '\n';
}

void print_error(std::string_view message, std::ostream &output) {
    output << "error: " << message << '\n';
}

void print_value(const Value &value, std::ostream &output) {
    if (value.type == LogicalType::Int) {
        output << value.as_int();
        return;
    }

    output << value.as_text();
}

void print_execution_result(const ExecutionResult &result, std::ostream &output) {
    if (result.kind == ExecutionResult::Kind::Ok) {
        output << "OK\n";
        return;
    }

    for (std::size_t index = 0; index < result.column_names.size(); ++index) {
        if (index > 0) {
            output << '\t';
        }
        output << result.column_names[index];
    }
    output << '\n';

    for (const std::vector<Value> &row : result.rows) {
        for (std::size_t index = 0; index < row.size(); ++index) {
            if (index > 0) {
                output << '\t';
            }
            print_value(row[index], output);
        }
        output << '\n';
    }
}

} // namespace

Repl::Repl(std::string database_name) : session_(cluster_, std::move(database_name)) {}

void Repl::process_line(const std::string &line, std::ostream &output) {
    if (line.empty()) {
        return;
    }

    if (is_help_command(line)) {
        print_help(output);
        return;
    }

    if (is_connect_command(line)) {
        const std::string_view database_name = connect_target(line);
        if (database_name.empty()) {
            print_error("database name required", output);
            return;
        }

        if (const Status status = session_.connect(database_name); !status.has_value()) {
            print_error(status.error().message, output);
            return;
        }

        output << "OK\n";
        return;
    }

    Parser parser(line);
    ParseResult<Statement> parsed = parser.parse_statement();
    if (!parsed.has_value()) {
        print_parse_error(parsed.error(), output);
        return;
    }

    Binder binder(session_);
    Result<BoundStatement> bound = binder.bind(std::move(parsed.value()));
    if (!bound.has_value()) {
        print_error(bound.error().message, output);
        return;
    }

    Executor executor(session_);
    Result<ExecutionResult> result = executor.execute(std::move(bound.value()));
    if (!result.has_value()) {
        print_error(result.error().message, output);
        return;
    }

    print_execution_result(result.value(), output);
}

int Repl::run(std::istream &input, std::ostream &output) {
    output << "DuraDB interactive shell. Type .help for commands.\n";
    output << "Connected to database '" << session_.current_database() << "'.\n";

    while (true) {
        output << "duradb> ";
        output.flush();

        std::string line;
        if (!std::getline(input, line)) {
            output << '\n';
            break;
        }

        if (is_exit_command(line)) {
            break;
        }

        process_line(line, output);
    }

    return 0;
}

} // namespace duradb
