cmake_minimum_required(VERSION 3.22)
project(5G_FastPacketParser CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(parser
src/main.cpp
src/MemoryMappedFile.cpp
)

target_include_directories(parser PRIVATE include)
