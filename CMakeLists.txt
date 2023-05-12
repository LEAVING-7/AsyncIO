cmake_minimum_required(VERSION 3.20)
project(AsyncIo VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-fcolor-diagnostics")

include(FetchContent)
macro(AddExternal target url git_tag)
  FetchContent_Declare(
    ${target}
    GIT_REPOSITORY https://ghproxy.com/https://github.com/${url}
    GIT_TAG ${git_tag}
  )
  FetchContent_MakeAvailable(${target})
endmacro()

AddExternal()

file(GLOB SOURCES "lib/*.cpp")

add_library(AsyncIo STATIC)
target_sources(AsyncIo PRIVATE ${SOURCES})
target_include_directories(AsyncIo
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
)

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  target_compile_definitions(AsyncIo PRIVATE DEBUG)
endif()

