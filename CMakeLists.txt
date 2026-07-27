cmake_minimum_required(VERSION 3.16)
project(duradb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(duradb src/main.cpp)

target_compile_options(duradb PRIVATE -Wall -Wextra -Wpedantic)
