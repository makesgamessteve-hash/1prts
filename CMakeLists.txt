cmake_minimum_required(VERSION 3.24)
project(1stprts VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PRTS_BUILD_TESTS "Build the engine's unit tests" ON)
option(PRTS_USE_SYSTEM_SDL3 "Use an installed SDL3 instead of fetching it" OFF)
option(PRTS_COVERAGE "Instrument engine code for coverage (GCC/Clang)" OFF)
option(PRTS_DEV "Developer features (source-tree data fallback, test hooks)" ON)
set(PRTS_SDL3_TAG "release-3.4.18" CACHE STRING "SDL3 git tag to fetch")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(PrtsWarnings)

add_subdirectory(third_party)

if(PRTS_USE_SYSTEM_SDL3)
  find_package(SDL3 REQUIRED CONFIG)
else()
  include(FetchContent)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG ${PRTS_SDL3_TAG}
    GIT_SHALLOW TRUE
    SYSTEM)
  FetchContent_MakeAvailable(SDL3)
endif()

add_subdirectory(engine)
add_subdirectory(apps/1stprts)

if(PRTS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(engine/tests)
endif()
