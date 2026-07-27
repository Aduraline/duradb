#include "support/token_test_utils.hpp"

#include <gtest/gtest.h>

using duradb::TokenKind;
using duradb::test::expect_token_stream;
using duradb::test::first_token;

TEST(LexerTest, TokenizesSelectStatement) {
    expect_token_stream("SELECT name FROM users WHERE age > 18;", {
                                                                      TokenKind::Select,
                                                                      TokenKind::Identifier,
                                                                      TokenKind::From,
                                                                      TokenKind::Identifier,
                                                                      TokenKind::Where,
                                                                      TokenKind::Identifier,
                                                                      TokenKind::Greater,
                                                                      TokenKind::IntegerLiteral,
                                                                      TokenKind::Semicolon,
                                                                      TokenKind::EndOfFile,
                                                                  });
}

TEST(LexerTest, TokenizesCreateTableStatement) {
    expect_token_stream("CREATE TABLE users (id INT, name TEXT);", {
                                                                       TokenKind::Create,
                                                                       TokenKind::Table,
                                                                       TokenKind::Identifier,
                                                                       TokenKind::LParen,
                                                                       TokenKind::Identifier,
                                                                       TokenKind::Int,
                                                                       TokenKind::Comma,
                                                                       TokenKind::Identifier,
                                                                       TokenKind::Text,
                                                                       TokenKind::RParen,
                                                                       TokenKind::Semicolon,
                                                                       TokenKind::EndOfFile,
                                                                   });
}

TEST(LexerTest, TokenizesInsertStatement) {
    expect_token_stream("INSERT INTO users VALUES (1, 'Alice');", {
                                                                      TokenKind::Insert,
                                                                      TokenKind::Into,
                                                                      TokenKind::Identifier,
                                                                      TokenKind::Values,
                                                                      TokenKind::LParen,
                                                                      TokenKind::IntegerLiteral,
                                                                      TokenKind::Comma,
                                                                      TokenKind::StringLiteral,
                                                                      TokenKind::RParen,
                                                                      TokenKind::Semicolon,
                                                                      TokenKind::EndOfFile,
                                                                  });
}

TEST(LexerTest, MatchesKeywordsCaseInsensitively) {
    expect_token_stream("select NaMe from USERS", {
                                                      TokenKind::Select,
                                                      TokenKind::Identifier,
                                                      TokenKind::From,
                                                      TokenKind::Identifier,
                                                      TokenKind::EndOfFile,
                                                  });
}

TEST(LexerTest, TokenizesComparisonOperators) {
    expect_token_stream("SELECT * FROM t WHERE a >= 1 AND b <> 'x' AND c != 'y';",
                        {
                            TokenKind::Select,
                            TokenKind::Star,
                            TokenKind::From,
                            TokenKind::Identifier,
                            TokenKind::Where,
                            TokenKind::Identifier,
                            TokenKind::GreaterEqual,
                            TokenKind::IntegerLiteral,
                            TokenKind::And,
                            TokenKind::Identifier,
                            TokenKind::NotEqual,
                            TokenKind::StringLiteral,
                            TokenKind::And,
                            TokenKind::Identifier,
                            TokenKind::NotEqual,
                            TokenKind::StringLiteral,
                            TokenKind::Semicolon,
                            TokenKind::EndOfFile,
                        });
}

TEST(LexerTest, TokenizesEscapedStringLiterals) {
    duradb::Lexer lexer("('O''Brien')");
    EXPECT_EQ(lexer.next().kind, TokenKind::LParen);

    const duradb::Token string_token = lexer.next();
    EXPECT_EQ(string_token.kind, TokenKind::StringLiteral);
    EXPECT_EQ(string_token.lexeme, "'O''Brien'");
}

TEST(LexerTest, SkipsLineComments) {
    expect_token_stream("SELECT 1; -- comment", {
                                                    TokenKind::Select,
                                                    TokenKind::IntegerLiteral,
                                                    TokenKind::Semicolon,
                                                    TokenKind::EndOfFile,
                                                });
}

TEST(LexerTest, ReportsInvalidCharacter) {
    const duradb::Token token = first_token("@");
    EXPECT_EQ(token.kind, TokenKind::Invalid);
    EXPECT_EQ(token.lexeme, "@");
}

TEST(LexerTest, TracksTokenPosition) {
    const duradb::Token select_token = first_token("SELECT name");
    EXPECT_EQ(select_token.kind, TokenKind::Select);
    EXPECT_EQ(select_token.line, 1U);
    EXPECT_EQ(select_token.column, 1U);

    duradb::Lexer lexer("SELECT name");
    lexer.next();
    const duradb::Token name_token = lexer.next();
    EXPECT_EQ(name_token.kind, TokenKind::Identifier);
    EXPECT_EQ(name_token.line, 1U);
    EXPECT_EQ(name_token.column, 8U);
}
