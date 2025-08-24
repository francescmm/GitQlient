cmake_minimum_required(VERSION 3.21)

project(test VERSION 0.1 LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Test)

qt_standard_project_setup()

add_subdirectory(Tests_Core)

## Addig Catch2
#add_subdirectory(deps/catch2)
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tests/catch2/extras")
#include(CTest)
#include(Catch)
#
#add_subdirectory(tests_cache)
add_subdirectory(Tests_Graph)
