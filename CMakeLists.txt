cmake_minimum_required(VERSION 3.20)
project(AsyncIO VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "-fcolor-diagnostics ${CMAKE_CXX_FLAGS}")
endif()

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "-fdiagnostics-color=always ${CMAKE_CXX_FLAGS}")
endif()

include(FetchContent)
macro(AddExternal target url git_tag)
  if(NOT DEFINED GLOBAL_GITHUB_ADDRESS)
    set(GLOBAL_GITHUB_ADDRESS "https://github.com")
  endif()
  FetchContent_Declare(
    ${target}
    GIT_REPOSITORY https://github.com/${url}
    GIT_TAG ${git_tag}
  )
  FetchContent_MakeAvailable(${target})
endmacro()

file(GLOB AsyncIO_SOURCES 
    "lib/*.cpp"
    "lib/sys/*.cpp"
    "lib/sys/unix/*.cpp"
    "lib/sys/win/*.cpp"
)

add_library(AsyncIO STATIC)
target_sources(AsyncIO PRIVATE ${AsyncIO_SOURCES})
target_include_directories(AsyncIO
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
)
target_link_libraries(AsyncIO PUBLIC AsyncTask)

# find openssl
find_package(OpenSSL REQUIRED)
target_link_libraries(AsyncIO PUBLIC OpenSSL::SSL OpenSSL::Crypto)

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "-fsanitize=address ${CMAKE_CXX_FLAGS}") 
  target_compile_definitions(AsyncIO PRIVATE DEBUG)
endif()
add_subdirectory(submodule)
option(AsyncIO_BUILD_TESTS "Build tests" OFF)
option(AsyncIO_BUILD_EXAMPLES "Build examples" OFF)

if(${AsyncIO_BUILD_TESTS})
  add_subdirectory(tests)
endif()
if(${AsyncIO_BUILD_EXAMPLES})
  add_subdirectory(examples)
endif()