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
duradb> CREATE TABLE users (id INT, name TEXT);
parsed successfully
duradb> SELECT * FROM users;
parsed successfully
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
│   ├── catalog/        # schema registry and binder
│   └── common/         # shared result types
└── tests/              # unit tests (GoogleTest)
```

## License

TBD
