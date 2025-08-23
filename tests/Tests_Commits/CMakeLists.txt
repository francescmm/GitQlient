cmake_minimum_required(VERSION 3.16)

project(Tests_Commits LANGUAGES CXX)

enable_testing()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Test)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB TEST_SOURCES
    *.cpp
)

add_executable(
    Tests_Commits
    ${TEST_SOURCES})

add_test(
    NAME Tests_Commits
    COMMAND Tests_Commits)

target_link_libraries(
    Tests_Commits
    PRIVATE
    Qt::Gui
    Qt::Test
    commits
)

target_include_directories(Tests_Commits PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
