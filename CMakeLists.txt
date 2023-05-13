cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_COMPILER /usr/bin/gcc-11)
set(CMAKE_CXX_FLAGS "-Wall -lm -lstdc++")
project(AsyncIO VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "-fcolor-diagnostics ${CMAKE_CXX_FLAGS}")
endif()

include(FetchContent)
macro(AddExternal target url git_tag)
  FetchContent_Declare(
    ${target}
    GIT_REPOSITORY https://github.com/${url}
    GIT_TAG ${git_tag}
  )
  FetchContent_MakeAvailable(${target})
endmacro()

# AddExternal(AsyncTask; LEAVING-7/AsyncTask.git; v0.1.1)

file(GLOB SOURCES 
    "lib/*.cpp"
    "lib/sys/*.cpp"
    "lib/sys/unix/*.cpp"
    "lib/sys/win/*.cpp"
)

add_library(AsyncIO STATIC)
target_sources(AsyncIO PRIVATE ${SOURCES})
target_include_directories(AsyncIO
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
)
target_link_libraries(AsyncIO PUBLIC AsyncTask)


if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "-fsanitize=address ${CMAKE_CXX_FLAGS}") 
  target_compile_definitions(AsyncIO PRIVATE DEBUG)
endif()

add_subdirectory(submodule/AsyncTask)

add_subdirectory(tests)
add_subdirectory(examples)
