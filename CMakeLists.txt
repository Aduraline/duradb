cmake_minimum_required(VERSION 3.16)
project(duradb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(duradb_frontend STATIC
    src/frontend/lexer.cpp
    src/frontend/parser.cpp
)
target_include_directories(duradb_frontend PUBLIC src)
target_compile_options(duradb_frontend PRIVATE -Wall -Wextra -Wpedantic)

add_library(duradb_engine STATIC
    src/catalog/schema.cpp
    src/catalog/value.cpp
    src/catalog/bound_expr.cpp
    src/catalog/binder.cpp
    src/engine/database_engine.cpp
    src/storage/row.cpp
)
target_include_directories(duradb_engine PUBLIC src)
target_link_libraries(duradb_engine PUBLIC duradb_frontend)
target_compile_options(duradb_engine PRIVATE -Wall -Wextra -Wpedantic)

add_executable(duradb
    src/main.cpp
    src/repl/repl.cpp
)
target_link_libraries(duradb PRIVATE duradb_engine)
target_compile_options(duradb PRIVATE -Wall -Wextra -Wpedantic)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(lexer_test
    tests/lexer_test.cpp
)
target_include_directories(lexer_test PRIVATE tests)
target_link_libraries(lexer_test PRIVATE duradb_frontend GTest::gtest_main)
target_compile_options(lexer_test PRIVATE -Wall -Wextra -Wpedantic)

include(GoogleTest)
gtest_discover_tests(lexer_test)

add_executable(parser_test
    tests/parser_test.cpp
)
target_include_directories(parser_test PRIVATE tests)
target_link_libraries(parser_test PRIVATE duradb_frontend GTest::gtest_main)
target_compile_options(parser_test PRIVATE -Wall -Wextra -Wpedantic)

gtest_discover_tests(parser_test)

add_executable(engine_test
    tests/engine_test.cpp
)
target_include_directories(engine_test PRIVATE tests)
target_link_libraries(engine_test PRIVATE duradb_engine GTest::gtest_main)
target_compile_options(engine_test PRIVATE -Wall -Wextra -Wpedantic)

gtest_discover_tests(engine_test)

add_executable(binder_test
    tests/binder_test.cpp
)
target_include_directories(binder_test PRIVATE tests)
target_link_libraries(binder_test PRIVATE duradb_engine GTest::gtest_main)
target_compile_options(binder_test PRIVATE -Wall -Wextra -Wpedantic)

gtest_discover_tests(binder_test)

add_executable(bound_expr_test
    tests/bound_expr_test.cpp
)
target_include_directories(bound_expr_test PRIVATE tests)
target_link_libraries(bound_expr_test PRIVATE duradb_engine GTest::gtest_main)
target_compile_options(bound_expr_test PRIVATE -Wall -Wextra -Wpedantic)

gtest_discover_tests(bound_expr_test)

add_executable(repl_test
    tests/repl_test.cpp
    src/repl/repl.cpp
)
target_include_directories(repl_test PRIVATE tests)
target_link_libraries(repl_test PRIVATE duradb_engine GTest::gtest_main)
target_compile_options(repl_test PRIVATE -Wall -Wextra -Wpedantic)

gtest_discover_tests(repl_test)

add_executable(row_test
    tests/row_test.cpp
)
target_link_libraries(row_test PRIVATE duradb_engine GTest::gtest_main)
target_compile_options(row_test PRIVATE -Wall -Wextra -Wpedantic)

gtest_discover_tests(row_test)

add_executable(value_test
    tests/value_test.cpp
)
target_link_libraries(value_test PRIVATE duradb_engine GTest::gtest_main)
target_compile_options(value_test PRIVATE -Wall -Wextra -Wpedantic)

gtest_discover_tests(value_test)

add_executable(storage_test
    tests/storage_test.cpp
)
target_link_libraries(storage_test PRIVATE duradb_engine GTest::gtest_main)
target_compile_options(storage_test PRIVATE -Wall -Wextra -Wpedantic)

gtest_discover_tests(storage_test)
