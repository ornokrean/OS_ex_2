cmake_minimum_required(VERSION 3.12)
project(Ex2)

set(CMAKE_CXX_STANDARD 14)

add_library(Ex2 uthreads.cpp Thread.cpp Thread.h my_test.cpp)