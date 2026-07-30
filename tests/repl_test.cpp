#include "repl/repl.hpp"

#include <gtest/gtest.h>
#include <sstream>

using duradb::Repl;

TEST(ReplTest, ExecutesValidStatements) {
    Repl repl;
    std::ostringstream output;

    repl.process_line("CREATE TABLE users (id INT, name TEXT);", output);
    repl.process_line("INSERT INTO users VALUES (1, 'Alice');", output);
    repl.process_line("SELECT name FROM users;", output);

    EXPECT_EQ(output.str(), "OK\nOK\nname\nAlice\n");
}

TEST(ReplTest, AcceptsSingleLineWithoutSemicolon) {
    Repl repl;
    std::ostringstream output;

    repl.process_line("CREATE TABLE users (id INT, name TEXT)", output);
    repl.process_line("INSERT INTO users VALUES (1, 'Alice')", output);
    repl.process_line("SELECT name FROM users", output);

    EXPECT_EQ(output.str(), "OK\nOK\nname\nAlice\n");
}

TEST(ReplTest, RequiresSemicolonForMultiLineInput) {
    Repl repl;
    std::istringstream input("CREATE TABLE users (\nid INT)\n");
    std::ostringstream output;

    repl.run(input, output);
    EXPECT_NE(output.str().find("expected ';'"), std::string::npos);
}

TEST(ReplTest, AcceptsMultiLineInputEndingWithSemicolon) {
    Repl repl;
    std::istringstream input(
        "CREATE TABLE users (\nid INT, name TEXT\n);\nSELECT name FROM users;\n.quit\n");
    std::ostringstream output;

    repl.run(input, output);
    EXPECT_NE(output.str().find("OK"), std::string::npos);
    EXPECT_EQ(output.str().find("parse error"), std::string::npos);
}

TEST(ReplTest, ReportsParseError) {
    Repl repl;
    std::ostringstream output;

    repl.process_line("SELECT FROM users;", output);
    EXPECT_NE(output.str().find("parse error"), std::string::npos);
}

TEST(ReplTest, ReportsExecutionError) {
    Repl repl;
    std::ostringstream output;

    repl.process_line("SELECT name FROM users;", output);
    EXPECT_NE(output.str().find("error:"), std::string::npos);
}

TEST(ReplTest, ConnectsToAnotherDatabase) {
    Repl repl;
    std::ostringstream output;

    repl.process_line("CREATE DATABASE app;", output);
    repl.process_line("CREATE TABLE users (id INT);", output);
    repl.process_line(".connect app", output);
    repl.process_line("CREATE TABLE users (id INT);", output);

    EXPECT_NE(output.str().find("OK"), std::string::npos);
    EXPECT_EQ(output.str().find("error:"), std::string::npos);
}
