cmake_minimum_required(VERSION 3.21)
project(GitQlient VERSION 0.1 LANGUAGES CXX)

option(BUILD_TESTS "Build the tests instead of the application" OFF)

add_subdirectory(app)
add_subdirectory(tests)
