cmake_minimum_required(VERSION 3.5)

project(cpp-hack LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(FetchContent)
function(add_onion)
  fetchcontent_declare(
    onion
    GIT_REPOSITORY git@git.koal.com:gw-client/seaworth/onion.git
    GIT_TAG develop
  )

  fetchcontent_getproperties(onion)
  if(NOT onion_POPULATED)
    fetchcontent_populate(onion)

    # default static library
    # set(BUILD_SHARED_LIBS ON) # for shared library
    # set(ONION_HEADER_ONLY ON) # for header only

    # use static boost
    # set(ONION_USE_STATIC_BOOST ON)

    # set(ONION_TRY_SYSTEM_OPENSSL ON) # default OFF
    # set(ONION_USE_SHARED_OPENSSL ON) # default OFF

    # set(ONION_TEST ON)  # default OFF
    add_subdirectory(${onion_SOURCE_DIR}/onion ${CMAKE_CURRENT_BINARY_DIR}/onion)
  endif()
endfunction(add_onion)

add_onion()

add_executable(cpp-hack main.cc)
target_compile_options(
  cpp-hack
  PRIVATE
  -W -Wall -Wextra -pedantic -ftemplate-backtrace-limit=0
)

target_link_libraries(
  cpp-hack
  onion
  # fmt
)

add_executable(c-hack main.c)

# add_executable(rx-test rx_test.cc)
# target_link_libraries(rx-test Threads::Threads)
