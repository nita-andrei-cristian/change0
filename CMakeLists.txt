cmake_minimum_required(VERSION 3.16)
project(change)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for lsp

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(node SHARED ${CMAKE_SOURCE_DIR}/c/lib/ne/node.c)
add_library(util SHARED ${CMAKE_SOURCE_DIR}/c/lib/util.c)

target_include_directories(node PUBLIC ${CMAKE_SOURCE_DIR}/c/lib/ne)
target_include_directories(util PUBLIC ${CMAKE_SOURCE_DIR}/c/lib)

add_executable(change ${CMAKE_SOURCE_DIR}/c/main.c)

target_link_libraries(change PRIVATE node util)
# target_link_libraries(change PRIVATE llama ggml)
