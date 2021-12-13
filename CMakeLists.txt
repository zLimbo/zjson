message("\n==== [zjson] ====")
cmake_minimum_required(VERSION 3.10)

project(zjson)

set(CMAKE_CXX_STANDARD 17)

aux_source_directory(. SOURCES)

add_executable(${PROJECT_NAME} ${SOURCES})
