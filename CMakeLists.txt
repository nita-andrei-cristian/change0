cmake_minimum_required(VERSION 3.16)
project(change)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for lsp

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(node SHARED ${CMAKE_SOURCE_DIR}/c/lib/ne/node.c)
add_library(search SHARED ${CMAKE_SOURCE_DIR}/c/lib/ne/search.c)
add_library(util SHARED ${CMAKE_SOURCE_DIR}/c/lib/util.c)
add_library(hd SHARED ${CMAKE_SOURCE_DIR}/c/lib/hd/hashdict.c)
add_library(ui SHARED ${CMAKE_SOURCE_DIR}/c/cli/ui.c)

target_include_directories(node PUBLIC ${CMAKE_SOURCE_DIR}/c/lib/ne)
target_include_directories(search PUBLIC ${CMAKE_SOURCE_DIR}/c/lib/ne)
target_include_directories(util PUBLIC ${CMAKE_SOURCE_DIR}/c/lib)
target_include_directories(hd PUBLIC ${CMAKE_SOURCE_DIR}/c/lib/hd)
target_include_directories(ui PUBLIC ${CMAKE_SOURCE_DIR}/c/cli)

add_executable(change ${CMAKE_SOURCE_DIR}/c/main.c)

target_link_libraries(change PRIVATE node util hd search ui)
# target_link_libraries(change PRIVATE llama ggml)
