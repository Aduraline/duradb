cmake_minimum_required(VERSION 3.16)
project(duradb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(duradb_frontend STATIC
    src/frontend/lexer.cpp
)
target_include_directories(duradb_frontend PUBLIC src)
target_compile_options(duradb_frontend PRIVATE -Wall -Wextra -Wpedantic)

add_executable(duradb
    src/main.cpp
)
target_link_libraries(duradb PRIVATE duradb_frontend)
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
