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
