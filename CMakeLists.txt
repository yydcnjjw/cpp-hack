cmake_minimum_required(VERSION 3.16)

project(cpp-hack LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(cpp-hack main.cc)
target_compile_options(
  cpp-hack
  PRIVATE
  -W -Wall -Wextra -pedantic -ftemplate-backtrace-limit=0
)

target_compile_definitions(
  cpp-hack
  PRIVATE
  "$<IF:$<CONFIG:Debug>,TEST_DEBUG,TEST_RELEASE>"
)

find_package(PkgConfig REQUIRED)

pkg_check_modules(BREAKPAD_CLIENT REQUIRED breakpad-client)
target_include_directories(
  cpp-hack
  PRIVATE
  ${BREAKPAD_CLIENT_INCLUDE_DIRS}
)

target_link_libraries(
  cpp-hack
  PRIVATE
  ${BREAKPAD_CLIENT_LIBRARIES}
)
