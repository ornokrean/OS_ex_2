cmake_minimum_required(VERSION 3.12)
project(Ex2)

set(CMAKE_CXX_STANDARD 11)

add_executable(Ex2 main.cpp uthreads.h uthreads.cpp thread.cpp thread.h sleeping_threads_list.h
        sleeping_threads_list.cpp )


