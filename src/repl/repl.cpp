#include "repl/repl.hpp"

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

void print_help(std::ostream &output) {
    output << "Commands:\n";
    output << "  .help       show this message\n";
    output << "  .quit       exit the shell\n";
    output << "SQL:\n";
    output << "  CREATE TABLE ...;\n";
    output << "  INSERT INTO ... VALUES (...);\n";
    output << "  SELECT ... FROM ... [WHERE ...];\n";
}

void print_parse_error(const ParseError &error, std::ostream &output) {
    output << "parse error at " << error.line << ':' << error.column << ": " << error.message
           << '\n';
}

} // namespace

void Repl::process_line(const std::string &line, std::ostream &output) {
    if (line.empty()) {
        return;
    }

    if (is_help_command(line)) {
        print_help(output);
        return;
    }

    Parser parser(line);
    const ParseResult<Statement> parsed = parser.parse_statement();
    if (!parsed.has_value()) {
        print_parse_error(parsed.error(), output);
        return;
    }

    // TODO: bind -> executor
    // TODO: print result sets for SELECT

    output << "parsed successfully\n";
}

int Repl::run(std::istream &input, std::ostream &output) {
    output << "DuraDB interactive shell. Type .help for commands.\n";

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
