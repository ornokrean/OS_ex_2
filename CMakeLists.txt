cmake_minimum_required(VERSION 3.12)
project(Ex2)

set(CMAKE_CXX_STANDARD 11)

add_executable(Ex2 ourTest3.cpp uthreads.h uthreads.cpp Thread.cpp Thread.h  sleeping_threads_list.h
        sleeping_threads_list.cpp)


