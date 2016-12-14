cmake_minimum_required(VERSION 2.8.9)
project(Mecacell)

set(CMAKE_CXX_FLAGS "-O3 -g -std=c++14 -Wall -Wextra -Wundef -Wshadow -Winit-self -Wconversion -Wformat=2 -pedantic ")
set(BUILD_TESTS "FALSE" CACHE BOOL "")

add_subdirectory(mecacell)
add_subdirectory(mecacellviewer)

if (BUILD_TESTS)
    message(STATUS "Building additional tests")
    add_subdirectory(tests)
endif(BUILD_TESTS)
