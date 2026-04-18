cmake_minimum_required(VERSION 3.16)
project(change)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for lsp

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)

add_library(hd SHARED ${CMAKE_SOURCE_DIR}/c/lib/hd/hashdict.c)
target_include_directories(hd PUBLIC ${CMAKE_SOURCE_DIR}/c/lib/hd)

add_library(jsonp SHARED ${CMAKE_SOURCE_DIR}/c/lib/jsonp/json.c)
target_include_directories(jsonp PUBLIC ${CMAKE_SOURCE_DIR}/c/lib/jsonp)
target_link_libraries(jsonp PRIVATE m) # add math

add_library(util SHARED ${CMAKE_SOURCE_DIR}/c/lib/util/util.c)
target_include_directories(util PUBLIC ${CMAKE_SOURCE_DIR}/c/lib/util)

add_library(openai SHARED ${CMAKE_SOURCE_DIR}/c/lib/openai/openai.c)
target_include_directories(openai PUBLIC ${CMAKE_SOURCE_DIR}/c/lib/openai)
target_link_libraries(openai PRIVATE OpenSSL::SSL OpenSSL::Crypto)

add_library(mockopenai SHARED ${CMAKE_SOURCE_DIR}/c/lib/openai/mockopenai.c)
target_include_directories(mockopenai PUBLIC ${CMAKE_SOURCE_DIR}/c/lib/openai)

add_library(ui SHARED ${CMAKE_SOURCE_DIR}/c/cli/ui.c)
target_include_directories(ui PUBLIC
    ${CMAKE_SOURCE_DIR}/c/cli   
    ${CMAKE_SOURCE_DIR}/c       
)
target_link_libraries(ui PUBLIC
    node
    util
    mockopenai
    graphexport
    inputprocessor
    deepsearchsession
)

add_library(graphexport SHARED ${CMAKE_SOURCE_DIR}/c/ne/graph/graph-export.c)
target_include_directories(graphexport PUBLIC
    ${CMAKE_SOURCE_DIR}/c/ne/graph
    ${CMAKE_SOURCE_DIR}/c/ne
)
target_link_libraries(graphexport PUBLIC
    util
    jsonp
    node
)

add_library(graphengine SHARED ${CMAKE_SOURCE_DIR}/c/ne/graph/graph-engine.c)
target_include_directories(graphengine PUBLIC
    ${CMAKE_SOURCE_DIR}/c/ne/graph
    ${CMAKE_SOURCE_DIR}/c/ne
)
target_link_libraries(graphengine PUBLIC node util)
target_link_libraries(graphengine PRIVATE m) 

add_library(node SHARED ${CMAKE_SOURCE_DIR}/c/ne/node.c)
target_include_directories(node PUBLIC ${CMAKE_SOURCE_DIR}/c/ne)
target_link_libraries(node PRIVATE m) 

add_library(mocks SHARED ${CMAKE_SOURCE_DIR}/c/ne/mocks/mocks.c)
target_include_directories(mocks PUBLIC
    ${CMAKE_SOURCE_DIR}/c/ne/mocks
    ${CMAKE_SOURCE_DIR}/c/lib/util
    ${CMAKE_SOURCE_DIR}/c/ne
    ${CMAKE_SOURCE_DIR}/c
)
target_link_libraries(mocks PUBLIC util node)

add_library(search SHARED ${CMAKE_SOURCE_DIR}/c/ne/search/search.c)
target_include_directories(search PUBLIC
    ${CMAKE_SOURCE_DIR}/c/ne/search
    ${CMAKE_SOURCE_DIR}/c/ne
)
target_link_libraries(search PUBLIC node util)

add_library(deepsearchsession SHARED ${CMAKE_SOURCE_DIR}/c/ne/search/deep-search-session.c)
target_include_directories(deepsearchsession PUBLIC
    ${CMAKE_SOURCE_DIR}/c/ne/search
    ${CMAKE_SOURCE_DIR}/c/lib/util
    ${CMAKE_SOURCE_DIR}/c/lib/jsonp
    ${CMAKE_SOURCE_DIR}/c/ne
    ${CMAKE_SOURCE_DIR}/c
)
target_link_libraries(deepsearchsession PUBLIC
    util
    jsonp
    comparsing
    graphengine
    mocks
)

add_library(deepsearchexecute SHARED ${CMAKE_SOURCE_DIR}/c/ne/search/deep-search-execute.c)
target_include_directories(deepsearchexecute PUBLIC
    ${CMAKE_SOURCE_DIR}/c/ne/search
    ${CMAKE_SOURCE_DIR}/c/lib/jsonp
    ${CMAKE_SOURCE_DIR}/c/ne
)
target_link_libraries(deepsearchexecute PUBLIC
    aiaction
    comparsing
    deepsearchsession
    jsonp
)

add_library(comparsing SHARED ${CMAKE_SOURCE_DIR}/c/ne/search/command-parsing.c)
target_include_directories(comparsing PUBLIC
    ${CMAKE_SOURCE_DIR}/c/ne/search
    ${CMAKE_SOURCE_DIR}/c/ne
)
target_link_libraries(comparsing PUBLIC
    util
    jsonp
)

add_library(aiaction SHARED ${CMAKE_SOURCE_DIR}/c/ne/search/ai-action.c)
target_include_directories(aiaction PUBLIC
    ${CMAKE_SOURCE_DIR}/c/ne/search
    ${CMAKE_SOURCE_DIR}/c/ne
)
target_link_libraries(aiaction PUBLIC
    jsonp
    util
    search
    comparsing
)

add_library(json2graph SHARED ${CMAKE_SOURCE_DIR}/c/ne/input/json-to-graph.c)
target_include_directories(json2graph PUBLIC
    ${CMAKE_SOURCE_DIR}/c/ne/input
    ${CMAKE_SOURCE_DIR}/c/ne
)
target_link_libraries(json2graph PUBLIC
    util
    jsonp
    node
)

add_library(inputprocessor SHARED ${CMAKE_SOURCE_DIR}/c/ne/input/input-processor.c)
target_include_directories(inputprocessor PUBLIC
    ${CMAKE_SOURCE_DIR}/c/ne/input
    ${CMAKE_SOURCE_DIR}/c/ne
    ${CMAKE_SOURCE_DIR}/c
)
target_link_libraries(inputprocessor PUBLIC
    openai
    util
    jsonp
    node
)

add_executable(change ${CMAKE_SOURCE_DIR}/c/main.c)
target_include_directories(change PRIVATE
    ${CMAKE_SOURCE_DIR}/c
    ${CMAKE_SOURCE_DIR}/c/ne
    ${CMAKE_SOURCE_DIR}/c/lib
    ${CMAKE_SOURCE_DIR}/c/lib/util
    ${CMAKE_SOURCE_DIR}/c/lib/openai
)

target_link_libraries(
	change PRIVATE

	hd jsonp node util ui 
	graphexport search deepsearchsession 
	deepsearchexecute openai 
	mockopenai mocks aiaction json2graph 
	graphengine inputprocessor
)
# target_link_libraries(change PRIVATE llama ggml)
