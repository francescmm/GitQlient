cmake_minimum_required(VERSION 3.16)

project(Tests_Graph LANGUAGES CXX)

enable_testing()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Test)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB TEST_SOURCES
    *.h
    *.cpp
)

add_executable(
    Tests_Graph
    ${TEST_SOURCES})

add_test(
    NAME Tests_Graph
    COMMAND Tests_Graph)

target_link_libraries(
    Tests_Graph
    PRIVATE
    Qt::Gui
    Qt::Test
    cache
)

target_include_directories(Tests_Graph PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
