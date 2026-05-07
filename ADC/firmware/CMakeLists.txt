cmake_minimum_required(VERSION 3.20)
project(adc_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(adc_core_demo
    src/adc_core.cpp
    src/main.cpp
)

target_include_directories(adc_core_demo PRIVATE include)

