cmake_minimum_required(VERSION 3.3)
project(untitled1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES test.cpp uthreads.cpp uthreads.h Thread.cpp Thread.h wasteBascket.cpp)
add_executable(untitled1 ${SOURCE_FILES})