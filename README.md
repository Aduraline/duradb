# DuraDB

A relational database engine in C++, built for high-throughput writes and low-latency reads. Early development; open source release planned.

## Requirements

- C++20 compiler (clang 13+, GCC 10+, or MSVC 19.29+)
- CMake 3.16+

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

Interactive shell:

```bash
./build/duradb
```

Example:

```text
DuraDB interactive shell. Type .help for commands.
duradb> CREATE TABLE users (id INT, name TEXT);
OK
duradb> INSERT INTO users VALUES (1, 'Adura');
OK
duradb> INSERT INTO users VALUES (2, 'Abraham');
OK
duradb> SELECT name FROM users WHERE id > 1;
Adura
Abraham
duradb> SELECT * FROM users;
id	name
1	Adura
2	Abraham
duradb> .quit
```

## Test

```bash
ctest --test-dir build --output-on-failure
```

## Development

Install [pre-commit](https://pre-commit.com/) once, then enable git hooks:

```bash
brew install pre-commit   # or: pip install pre-commit
pre-commit install
```

On each commit, hooks will:

- fix trailing whitespace and missing end-of-file newlines
- run `clang-format` on C++ sources
- build the project and run unit tests with CTest

Run hooks manually against all files:

```bash
pre-commit run --all-files
```

## Project layout

```
duradb/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── frontend/       # SQL frontend (lexer, parser, ast)
│   ├── catalog/        # schema, binder, and bound expressions
│   ├── engine/         # unified database engine
│   ├── storage/        # row types and validation
│   └── common/         # shared result types
└── tests/              # unit tests (GoogleTest)
```

## License

TBD
