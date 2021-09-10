cmake_minimum_required(VERSION 3.5)

project(cpp-test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(test SHARED test.cc)

add_executable(cpp-test main.cc)
target_link_libraries(cpp-test test)


