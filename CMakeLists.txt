cmake_minimum_required(VERSION 3.22)
project(5G_FastPacketParser CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# 1. Compile parser logic as a library once
add_library(parser_core
        src/MemoryMappedFile.cpp
        src/MacParser.cpp
    )
target_include_directories(parser_core PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 2. Main binary links the library
add_executable(parser src/main.cpp)
target_link_libraries(parser PRIVATE parser_core)

# 3. Google Test
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.zip
    )
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(parser_tests tests/test_MacSubheader.cpp)
target_link_libraries(parser_tests PRIVATE parser_core gtest_main)

include(GoogleTest)
gtest_discover_tests(parser_tests)

FetchContent_Declare(
        googlebenchmark
        URL https://github.com/google/benchmark/archive/refs/tags/v1.8.4.zip
    )
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(parser_benchmarks benchmarks/bench_parser.cpp)
target_link_libraries(parser_benchmarks PRIVATE parser_core
                      benchmark::benchmark_main)
