cmake_minimum_required(VERSION 3.5)
project(ch)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        ch.cpp)

add_executable(ch ${SOURCE_FILES})
