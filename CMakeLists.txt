cmake_minimum_required(VERSION 3.5)

project(cpp-test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Threads REQUIRED)

add_executable(cpp-test main.cc)
target_link_libraries(cpp-test Threads::Threads)


