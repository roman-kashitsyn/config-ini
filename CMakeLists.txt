cmake_minimum_required(VERSION 2.8)
project(config-ini)

option(build_tests "build unit tests" OFF)

include_directories(include)
add_library(${PROJECT_NAME} SHARED src/ini_parser.cpp)

find_package(Boost
  COMPONENTS unit_test_framework)

if (Boost_FOUND AND build_tests)

  add_executable(
    ${PROJECT_NAME}_test
    test/test_parser.cpp
    )

  target_link_libraries(
    ${PROJECT_NAME}_test
    ${PROJECT_NAME}
    ${Boost_unit_test_framework_LIBRARY_DEBUG}
    )

endif()