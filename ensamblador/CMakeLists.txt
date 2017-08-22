cmake_minimum_required(VERSION 3.5)
project(chasm)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        chasm.cpp)

add_executable(chasm ${SOURCE_FILES})
