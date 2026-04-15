cmake_minimum_required(VERSION 3.16)
project(change)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for lsp

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)

add_library(hd SHARED ${CMAKE_SOURCE_DIR}/c/lib/hd/hashdict.c)
add_library(jsonp SHARED ${CMAKE_SOURCE_DIR}/c/lib/jsonp/json.c)
add_library(node SHARED ${CMAKE_SOURCE_DIR}/c/ne/node.c)
add_library(util SHARED ${CMAKE_SOURCE_DIR}/c/lib/util/util.c)
add_library(openai SHARED ${CMAKE_SOURCE_DIR}/c/lib/openai/openai.c)
add_library(mockopenai SHARED ${CMAKE_SOURCE_DIR}/c/lib/openai/mockopenai.c)
add_library(ui SHARED ${CMAKE_SOURCE_DIR}/c/cli/ui.c)
add_library(engine SHARED ${CMAKE_SOURCE_DIR}/c/ne/engine.c)
add_library(search SHARED ${CMAKE_SOURCE_DIR}/c/ne/search.c)
add_library(deepsearchsession SHARED ${CMAKE_SOURCE_DIR}/c/ne/deep-search-session.c)
add_library(deepsearchexecute SHARED ${CMAKE_SOURCE_DIR}/c/ne/deep-search-execute.c)
add_library(comparsing SHARED ${CMAKE_SOURCE_DIR}/c/ne/command-parsing.c)
add_library(mocks SHARED ${CMAKE_SOURCE_DIR}/c/ne/mocks.c)
add_library(aiaction SHARED ${CMAKE_SOURCE_DIR}/c/ne/ai-action.c)

target_include_directories(hd PUBLIC ${CMAKE_SOURCE_DIR}/c/lib/hd)
target_include_directories(jsonp PUBLIC ${CMAKE_SOURCE_DIR}/c/lib/jsonp)
target_include_directories(node PUBLIC ${CMAKE_SOURCE_DIR}/c/ne/node)
target_include_directories(util PUBLIC ${CMAKE_SOURCE_DIR}/c/lib/util)
target_include_directories(openai PUBLIC ${CMAKE_SOURCE_DIR}/c/lib/openai)
target_include_directories(mockopenai PUBLIC ${CMAKE_SOURCE_DIR}/c/lib/openai)
target_include_directories(ui PUBLIC ${CMAKE_SOURCE_DIR}/c/cli)
target_include_directories(engine PUBLIC ${CMAKE_SOURCE_DIR}/c/ne)
target_include_directories(search PUBLIC ${CMAKE_SOURCE_DIR}/c/ne)
target_include_directories(deepsearchsession PUBLIC ${CMAKE_SOURCE_DIR}/c/ne)
target_include_directories(deepsearchexecute PUBLIC ${CMAKE_SOURCE_DIR}/c/ne)
target_include_directories(comparsing PUBLIC ${CMAKE_SOURCE_DIR}/c/ne)
target_include_directories(mocks PUBLIC ${CMAKE_SOURCE_DIR}/c/ne)
target_include_directories(aiaction PUBLIC ${CMAKE_SOURCE_DIR}/c/ne)

target_link_libraries(jsonp PRIVATE m) # add math
target_link_libraries(engine PRIVATE m) # add math
target_link_libraries(node PRIVATE m) # add math

target_link_libraries(openai PRIVATE OpenSSL::SSL OpenSSL::Crypto)

add_executable(change ${CMAKE_SOURCE_DIR}/c/main.c)

target_link_libraries(deepsearchsession comparsing)

target_link_libraries(change PRIVATE hd jsonp node util ui engine search deepsearchsession deepsearchexecute openai mockopenai mocks aiaction)
# target_link_libraries(change PRIVATE llama ggml)

