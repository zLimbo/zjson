message("\n==== [zjson/tutorial] ====")
cmake_minimum_required(VERSION 3.10)

project(zjson_test CXX)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -pedantic -Wall")
endif()

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(CMAKE_CXX_STANDARD 17)

add_library(zjson zjson.cpp)
add_executable(zjson_test zjson_test.cpp)
target_link_libraries(zjson_test zjson)

# add_executable(zjson_hpp_test zjson_hpp_test.cpp zjson.hpp)