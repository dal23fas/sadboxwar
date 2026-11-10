# EARTH FRONTIER — CMake build for the engine-neutral core, tests and headless tools.
# The same Source/EFWorldCore tree is also compiled by UnrealBuildTool (Source/EFWorldCore/EFWorldCore.Build.cs).
cmake_minimum_required(VERSION 3.16)
project(EarthFrontierCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(EF_UE_COMPAT_FLAGS "Compile with -fno-exceptions -fno-rtti to mirror UnrealBuildTool defaults" ON)

file(GLOB EF_CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/EFWorldCore/Private/*.cpp)
add_library(EFWorldCore STATIC ${EF_CORE_SOURCES})
target_include_directories(EFWorldCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source/EFWorldCore/Public)
target_compile_options(EFWorldCore PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-sign-conversion)
if(EF_UE_COMPAT_FLAGS)
  target_compile_options(EFWorldCore PUBLIC -fno-exceptions -fno-rtti)
endif()

# ---- headless acceptance tool (runs the vertical-slice script in data form) ----
add_executable(ef_headless Tools/ef_headless/main.cpp)
target_link_libraries(ef_headless PRIVATE EFWorldCore)

# ---- tests ----
enable_testing()
set(EF_CONTENT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Content/Data/asterion_island_v1.json")
foreach(t test_json test_events test_world_loop test_save_load test_simulation)
  add_executable(${t} Tests/${t}.cpp)
  target_link_libraries(${t} PRIVATE EFWorldCore)
  target_include_directories(${t} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Tests)
  target_compile_definitions(${t} PRIVATE EF_CONTENT_PATH="${EF_CONTENT_PATH}" EF_TEST_TMP="${CMAKE_CURRENT_BINARY_DIR}/tmp")
  add_test(NAME ${t} COMMAND ${t})
endforeach()
