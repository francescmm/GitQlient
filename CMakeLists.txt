cmake_minimum_required(VERSION 3.21)
project(GitQlient VERSION 2.0 LANGUAGES CXX)

option(BUILD_TESTS "Build the tests instead of the application" OFF)

add_subdirectory(app)
add_subdirectory(tests)
