cmake_minimum_required(VERSION 3.20)
project(mag_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(mag_core_demo
    src/mag_core.cpp
    src/main.cpp
)

target_include_directories(mag_core_demo PRIVATE include)

