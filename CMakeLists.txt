cmake_minimum_required(VERSION 3.3)
project(untitled1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES uthreads.cpp uthreads.h Thread.h Thread.cpp)
add_executable(untitled1 ${SOURCE_FILES})