cmake_minimum_required(VERSION 3.16)
project(change)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for lsp

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(hd SHARED ${CMAKE_SOURCE_DIR}/c/lib/hd/hashdict.c)
add_library(jsonp SHARED ${CMAKE_SOURCE_DIR}/c/lib/jsonp/json.c)
add_library(node SHARED ${CMAKE_SOURCE_DIR}/c/ne/node.c)
add_library(util SHARED ${CMAKE_SOURCE_DIR}/c/lib/util/util.c)
add_library(ui SHARED ${CMAKE_SOURCE_DIR}/c/cli/ui.c)
add_library(engine SHARED ${CMAKE_SOURCE_DIR}/c/ne/engine.c)
add_library(search SHARED ${CMAKE_SOURCE_DIR}/c/ne/search.c)

target_include_directories(hd PUBLIC ${CMAKE_SOURCE_DIR}/c/lib/hd)
target_include_directories(jsonp PUBLIC ${CMAKE_SOURCE_DIR}/c/lib/jsonp)
target_include_directories(node PUBLIC ${CMAKE_SOURCE_DIR}/c/ne/node)
target_include_directories(util PUBLIC ${CMAKE_SOURCE_DIR}/c/lib/util)
target_include_directories(ui PUBLIC ${CMAKE_SOURCE_DIR}/c/cli)
target_include_directories(engine PUBLIC ${CMAKE_SOURCE_DIR}/c/ne)
target_include_directories(search PUBLIC ${CMAKE_SOURCE_DIR}/c/ne)

target_link_libraries(jsonp PRIVATE m) # add math

add_executable(change ${CMAKE_SOURCE_DIR}/c/main.c)

target_link_libraries(change PRIVATE hd jsonp node util ui engine search)
# target_link_libraries(change PRIVATE llama ggml)
