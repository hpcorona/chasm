cmake_minimum_required(VERSION 3.5)
project(chd)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        chd.cpp)

add_executable(chd ${SOURCE_FILES})
