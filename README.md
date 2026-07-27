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

```bash
./build/duradb
```

## Project layout

```
duradb/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   └── frontend/       # SQL frontend (lexer, parser)
```

## License

TBD
