#include "repl/repl.hpp"

#include "catalog/binder.hpp"
#include "execution/executor.hpp"
#include "frontend/parser.hpp"

#include <cctype>
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

bool is_tables_command(std::string_view line) {
    return line == ".tables";
}

std::string_view connect_target(std::string_view line) {
    return line.substr(std::string_view(".connect ").size());
}

bool ends_with_semicolon(std::string_view sql) {
    while (!sql.empty() && std::isspace(static_cast<unsigned char>(sql.back()))) {
        sql.remove_suffix(1);
    }

    return !sql.empty() && sql.back() == ';';
}

int parenthesis_balance(std::string_view sql) {
    int balance = 0;

    for (const char character : sql) {
        if (character == '(') {
            ++balance;
        } else if (character == ')') {
            --balance;
        }
    }

    return balance;
}

bool needs_continuation(std::string_view sql) {
    if (ends_with_semicolon(sql)) {
        return false;
    }

    if (parenthesis_balance(sql) > 0) {
        return true;
    }

    while (!sql.empty() && std::isspace(static_cast<unsigned char>(sql.back()))) {
        sql.remove_suffix(1);
    }

    return !sql.empty() && (sql.back() == '(' || sql.back() == ',');
}

void print_help(std::ostream &output) {
    output << "Commands:\n";
    output << "  .help              show this message\n";
    output << "  .connect <db>      switch to another database\n";
    output << "  .tables             list all tables in the current database\n";
    output << "  .quit              exit the shell\n";
    output << "SQL:\n";
    output << "  CREATE DATABASE ...;\n";
    output << "  CREATE SCHEMA ...;\n";
    output << "  CREATE TABLE ...;\n";
    output << "  INSERT INTO ... VALUES (...);\n";
    output << "  SELECT ... FROM ... [WHERE ...];\n";
    output << "Semicolons are optional on single line statements.\n";
    output << "Multi line input must end with ';'.\n";
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

void Repl::process_sql(const std::string &sql, std::ostream &output) {
    Parser parser(sql);
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

bool Repl::read_sql_buffer(const std::string &first_line, std::istream &input, std::ostream &output,
                           std::string &buffer, bool &multiline) {
    buffer = first_line;
    multiline = false;

    if (!needs_continuation(buffer)) {
        return true;
    }

    multiline = true;

    while (true) {
        output << "duradb|> ";
        output.flush();

        std::string line;
        if (!std::getline(input, line)) {
            output << '\n';
            return false;
        }

        if (!buffer.empty()) {
            buffer.push_back(' ');
        }
        buffer += line;

        if (!needs_continuation(buffer)) {
            return true;
        }
    }
}

void Repl::process_line(const std::string &line, std::ostream &output) {
    if (line.empty()) {
        return;
    }

    if (is_help_command(line)) {
        print_help(output);
        return;
    }

   if (is_tables_command(line)) {
      const std::vector<std::string> tables = session_.list_tables();
      for (const auto &table : tables) {
          output << table << '\n';
      }
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

    process_sql(line, output);
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

        if (line.empty()) {
            continue;
        }

        if (is_help_command(line) || is_connect_command(line) || is_tables_command(line)) {
            process_line(line, output);
            continue;
        }

        std::string buffer;
        bool multiline = false;
        if (!read_sql_buffer(line, input, output, buffer, multiline)) {
            break;
        }

        if (multiline && !ends_with_semicolon(buffer)) {
            print_error("expected ';' to complete multi-line statement", output);
            continue;
        }

        process_sql(buffer, output);
    }

    return 0;
}

} // namespace duradb
