cmake_minimum_required(VERSION 3.5)
project(chc)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

find_program(CLANG_EXECUTABLE
        NAMES clang-3.7 clang-3.6 clang
        PATHS ENV LLVM_PATH)
if(CLANG_EXECUTABLE)
    message(STATUS "clang found : ${CLANG_EXECUTABLE}")
else()
    message(FATAL_ERROR "Can't found program: clang")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti")

set(SOURCE_FILES
        chc.cpp
        parser.cpp
        tokens.cpp
        types.cpp
        gc_chvm.cpp
        gc_llvm.cpp)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(chc ${SOURCE_FILES} ${CMAKE_BINARY_DIR}/runtime_bc.cpp)

add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/runtime_bc.cpp
        PRE_BUILD
        COMMAND ${CLANG_EXECUTABLE} -O0 -emit-llvm ${PROJECT_SOURCE_DIR}/../llvmruntime/runtime.cpp -c -o ${CMAKE_BINARY_DIR}/runtime.bc &&
            export CHC_RES_NAME=runtime_bc &&
            export CHC_RES_PATH=${CMAKE_BINARY_DIR}/runtime.bc &&
            export CHC_RES_OUTPUT=${CMAKE_BINARY_DIR}/runtime_bc.cpp &&
            ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/CreateResource.cmake
)

llvm_map_components_to_libnames(llvm_libs all)

target_link_libraries(chc ${llvm_libs})

