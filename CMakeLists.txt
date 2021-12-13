message("\n==== [zjson] ====")
cmake_minimum_required(VERSION 3.10)

project(zjson_test CXX)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -pedantic -Wall")
endif()

set(CMAKE_CXX_STANDARD 17)

add_library(zjson zjson.cpp)
add_executable(zjson_test zjson_test.cpp)
target_link_libraries(zjson_test zjson)
