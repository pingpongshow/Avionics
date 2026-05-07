cmake_minimum_required(VERSION 3.20)
project(eis_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(eis_core_demo
    src/main.cpp
    src/eis_core.cpp
)

target_include_directories(eis_core_demo PRIVATE include)

