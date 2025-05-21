cmake_minimum_required(VERSION 3.15)
project(pjrt_project CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --- PJRT Plugin and Python Environment Handling ---
# Include the custom module to detect Python environment and set PJRT paths
# This will define PJRT_PLUGIN_FULL_PATH_CONFIG and RPATH_ENTRIES_CONFIG
include(cmake/PJRTSettings.cmake)

# --- Dynamic Linking Library ---
if(NOT CMAKE_DL_LIBS)
    message(WARNING "CMAKE_DL_LIBS is not defined. Dynamic linking might fail. Trying to link with 'dl' manually.")
    find_library(DL_LIBRARY NAMES dl)
    if(NOT DL_LIBRARY)
        message(FATAL_ERROR "Failed to find the dynamic linking library (libdl). Please ensure it's installed.")
    endif()
    set(CMAKE_DL_LIBS ${DL_LIBRARY})
    message(STATUS "Using CMAKE_DL_LIBS: ${CMAKE_DL_LIBS}")
else()
    message(STATUS "CMAKE_DL_LIBS already defined: ${CMAKE_DL_LIBS}")
endif()

# --- Add Library Subdirectory ---
add_subdirectory(pjrt)

# --- Options to build examples and tests ---
option(BUILD_EXAMPLES "Build the example application" ON)
option(BUILD_TESTS "Build the tests for pjrt" ON) # Default to ON, can be set to OFF

if(BUILD_EXAMPLES)
    if(NOT PJRT_PLUGIN_FULL_PATH_CONFIG)
        message(WARNING "PJRT_PLUGIN_FULL_PATH_CONFIG is not set. Examples might not run correctly if they require a PJRT plugin.")
    endif()
    add_subdirectory(examples)
endif()

if(BUILD_TESTS)
    if(NOT PJRT_PLUGIN_FULL_PATH_CONFIG)
        message(WARNING "PJRT_PLUGIN_FULL_PATH_CONFIG is not set. Tests might not run correctly if they require a PJRT plugin.")
    endif()
    enable_testing() # Enables CTest
    add_subdirectory(tests)
endif()

message(STATUS "Configuration finished. Project: ${PROJECT_NAME}")
message(STATUS "To build, run 'cmake --build <build_dir>'")
if(BUILD_EXAMPLES)
    message(STATUS "Example executable can be found in '<build_dir>/examples/'")
endif()
if(BUILD_TESTS)
    message(STATUS "Tests can be run with 'ctest' from the build directory after building.")
endif()