cmake_minimum_required(VERSION 2.8)
project(config-ini)

include_directories(include)
add_library(ini src/ini_parser.cpp)

find_package(Boost
  COMPONENTS unit_test_framework)

if (Boost_FOUND)
  add_executable(ini_test
    test/test_parser.cpp)
  target_link_libraries(ini_test
    ini
    boost_unit_test_framework-mt)
endif()