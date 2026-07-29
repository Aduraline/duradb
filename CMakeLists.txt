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

add_library(duradb_catalog STATIC
    src/catalog/schema.cpp
    src/catalog/value.cpp
    src/catalog/catalog.cpp
    src/catalog/binder.cpp
)
target_include_directories(duradb_catalog PUBLIC src)
target_link_libraries(duradb_catalog PUBLIC duradb_frontend)
target_compile_options(duradb_catalog PRIVATE -Wall -Wextra -Wpedantic)

add_library(duradb_storage STATIC
    src/storage/row.cpp
    src/storage/heap_table.cpp
)
target_include_directories(duradb_storage PUBLIC src)
target_link_libraries(duradb_storage PUBLIC duradb_catalog)
target_compile_options(duradb_storage PRIVATE -Wall -Wextra -Wpedantic)

add_executable(duradb
    src/main.cpp
    src/repl/repl.cpp
)
target_link_libraries(duradb PRIVATE duradb_catalog)
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

add_executable(catalog_test
    tests/catalog_test.cpp
)
target_include_directories(catalog_test PRIVATE tests)
target_link_libraries(catalog_test PRIVATE duradb_catalog GTest::gtest_main)
target_compile_options(catalog_test PRIVATE -Wall -Wextra -Wpedantic)

gtest_discover_tests(catalog_test)

add_executable(binder_test
    tests/binder_test.cpp
)
target_include_directories(binder_test PRIVATE tests)
target_link_libraries(binder_test PRIVATE duradb_catalog GTest::gtest_main)
target_compile_options(binder_test PRIVATE -Wall -Wextra -Wpedantic)

gtest_discover_tests(binder_test)

add_executable(repl_test
    tests/repl_test.cpp
    src/repl/repl.cpp
)
target_include_directories(repl_test PRIVATE tests)
target_link_libraries(repl_test PRIVATE duradb_catalog GTest::gtest_main)
target_compile_options(repl_test PRIVATE -Wall -Wextra -Wpedantic)

gtest_discover_tests(repl_test)

add_executable(row_test
    tests/row_test.cpp
    src/storage/row.cpp
)
target_link_libraries(row_test PRIVATE duradb_storage GTest::gtest_main)
target_compile_options(row_test PRIVATE -Wall -Wextra -Wpedantic)

gtest_discover_tests(row_test)

add_executable(heap_table_test
    tests/heap_table_test.cpp
)
target_link_libraries(heap_table_test PRIVATE duradb_storage GTest::gtest_main)
target_compile_options(heap_table_test PRIVATE -Wall -Wextra -Wpedantic)

gtest_discover_tests(heap_table_test)
