cmake_minimum_required(VERSION 3.15)
project(pjrt_loader CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Define the name of your Python script
set(PYTHON_SCRIPT_NAME "myJax.py")
# Define the name for the output file
set(OUTPUT_HLO_FILE_NAME "stableHlo.program")

# Construct the full path to your Python script (assuming it's in the source directory)
set(PYTHON_SCRIPT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${PYTHON_SCRIPT_NAME}")

# Construct the full path for the output file (placing it in the build directory)
set(OUTPUT_HLO_FILE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_HLO_FILE_NAME}")

# Add a custom command that generates the StableHLO file.
# This command runs the Python script and uses shell redirection '>'
# to capture its standard output.
# VERBATIM is important to ensure the command string (especially the '>')
# is passed correctly to the underlying build tool (e.g., Make, Ninja),
# which will typically use a shell to execute it.
add_custom_command(
    OUTPUT ${OUTPUT_HLO_FILE_PATH}
    COMMAND
        ${Python3_EXECUTABLE} ${PYTHON_SCRIPT_PATH} > ${OUTPUT_HLO_FILE_PATH}
    DEPENDS
        ${PYTHON_SCRIPT_PATH} # The command depends on the Python script itself
        ${Python3_EXECUTABLE} # And on the interpreter
    MAIN_DEPENDENCY ${PYTHON_SCRIPT_PATH} # More explicit primary dependency
    COMMENT "Generating ${OUTPUT_HLO_FILE_NAME} from ${PYTHON_SCRIPT_NAME}"
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR} # Sets the current working directory for the script execution.
                                     # Use CMAKE_CURRENT_BINARY_DIR if more appropriate for your script.
    VERBATIM # Crucial for correct handling of special characters and arguments.
)

# Add a custom target to trigger the execution of the custom command.
# You can build this target explicitly, e.g., `cmake --build . --target GenerateStableHlo`
# or `make GenerateStableHlo` or `ninja GenerateStableHlo`.
add_custom_target(GenerateStableHlo ALL
    DEPENDS ${OUTPUT_HLO_FILE_PATH} # This target depends on the output file being created.
    COMMENT "Ensuring ${OUTPUT_HLO_FILE_NAME} is generated."
)

add_executable(pjrt_loader
  main.cpp
  pjrt/buffer.cpp
  pjrt/buffer.hpp
  pjrt/client.cpp
  pjrt/client.hpp
  pjrt/common.cpp
  pjrt/common.hpp
  pjrt/context.cpp
  pjrt/context.hpp
  pjrt/deviceView.cpp
  pjrt/deviceView.hpp
  pjrt/event.cpp
  pjrt/event.hpp
  pjrt/loadedExecutable.cpp
  pjrt/loadedExecutable.hpp
)

target_include_directories(pjrt_loader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}) # For pjrt_c_api.h

if(NOT CMAKE_DL_LIBS)
    message(WARNING "CMAKE_DL_LIBS is not defined. Dynamic linking might fail. Trying to link with 'dl' manually.")
    find_library(DL_LIBRARY NAMES dl)
    if(NOT DL_LIBRARY)
        message(FATAL_ERROR "Failed to find the dynamic linking library (libdl). Please ensure it's installed.")
    endif()
    set(CMAKE_DL_LIBS ${DL_LIBRARY})
endif()
target_link_libraries(pjrt_loader PRIVATE ${CMAKE_DL_LIBS})

# --- Detect .venv Python specific paths ---
# Find the python3.x directory within .venv/lib/
file(GLOB VENV_PYTHON_LIB_CANDIDATES "${CMAKE_CURRENT_SOURCE_DIR}/.venv/lib/python3.*")

set(VENV_PYTHON_LIB_DIR "")
if(VENV_PYTHON_LIB_CANDIDATES)
    # Use the first one found (usually there's only one in a venv)
    list(GET VENV_PYTHON_LIB_CANDIDATES 0 VENV_PYTHON_LIB_DIR)
    if(NOT IS_DIRECTORY ${VENV_PYTHON_LIB_DIR})
        message(FATAL_ERROR "Detected venv Python lib directory is not a directory: ${VENV_PYTHON_LIB_DIR}")
    endif()
    message(STATUS "Detected venv Python library directory: ${VENV_PYTHON_LIB_DIR}")
else()
    message(FATAL_ERROR "Could not find a python3.x directory in .venv/lib/. Please ensure '.venv' is created and 'jax[cuda]' is installed.")
endif()

set(VENV_SITE_PACKAGES_DIR "${VENV_PYTHON_LIB_DIR}/site-packages")
if(NOT IS_DIRECTORY ${VENV_SITE_PACKAGES_DIR})
    message(FATAL_ERROR "Venv site-packages directory not found: ${VENV_SITE_PACKAGES_DIR}")
endif()

# --- Define PJRT Plugin Path & RPATHs based on .venv ---
set(PJRT_PLUGIN_DIR "${VENV_SITE_PACKAGES_DIR}/jax_plugins/xla_cuda12") # Assuming CUDA 12 plugin
set(PJRT_PLUGIN_FULL_PATH "${PJRT_PLUGIN_DIR}/xla_cuda_plugin.so")

set(CUDNN_LIB_DIR "${VENV_SITE_PACKAGES_DIR}/nvidia/cudnn/lib")
set(CUDA_RUNTIME_LIB_DIR "${VENV_SITE_PACKAGES_DIR}/nvidia/cuda_runtime/lib")
set(JAXLIB_DIR "${VENV_SITE_PACKAGES_DIR}/jaxlib") # For libjax_common.so etc.

# Check existence of critical paths
if(NOT EXISTS "${PJRT_PLUGIN_FULL_PATH}")
    message(FATAL_ERROR "PJRT Plugin (xla_cuda_plugin.so) not found at expected .venv path: ${PJRT_PLUGIN_FULL_PATH}. Ensure jax[cuda] is installed correctly and installs the xla_cuda12 plugin.")
endif()
if(NOT IS_DIRECTORY ${CUDNN_LIB_DIR})
    message(WARNING "CUDNN lib directory for RPATH not found in .venv: ${CUDNN_LIB_DIR}")
endif()
if(NOT IS_DIRECTORY ${CUDA_RUNTIME_LIB_DIR})
    message(WARNING "CUDA Runtime lib directory for RPATH not found in .venv: ${CUDA_RUNTIME_LIB_DIR}")
endif()
if(NOT IS_DIRECTORY ${JAXLIB_DIR})
    message(WARNING "JAXLIB directory for RPATH not found in .venv: ${JAXLIB_DIR}")
endif()

# Pass the plugin path to the C++ code
target_compile_definitions(pjrt_loader PRIVATE
  "PJRT_PLUGIN_PATH=\"${PJRT_PLUGIN_FULL_PATH}\""
  "STABLEHLO_PROGRAM_PATH=\"${OUTPUT_HLO_FILE_PATH}\""
)

# Set RPATH for the executable
set(CMAKE_MACOSX_RPATH OFF)

set(RPATH_ENTRIES
    "${PJRT_PLUGIN_DIR}"        # Directory of the plugin itself
    "${CUDNN_LIB_DIR}"
    "${CUDA_RUNTIME_LIB_DIR}"
    "${JAXLIB_DIR}"
)

# For Linux/Unix-like linkers
set(RPATH_LINKER_FLAGS "")
foreach(RPATH_DIR ${RPATH_ENTRIES})
    # Ensure paths are absolute for RPATH, CMake usually handles this if they are derived from CMAKE_CURRENT_SOURCE_DIR
    if(IS_ABSOLUTE "${RPATH_DIR}")
        list(APPEND RPATH_LINKER_FLAGS "-Wl,-rpath,${RPATH_DIR}")
    else()
        list(APPEND RPATH_LINKER_FLAGS "-Wl,-rpath,${CMAKE_CURRENT_SOURCE_DIR}/${RPATH_DIR}") # Should not happen with current logic
    endif()
endforeach()

target_link_options(pjrt_loader PRIVATE ${RPATH_LINKER_FLAGS})

message(STATUS "Target executable: pjrt_loader")
message(STATUS "PJRT Plugin to be loaded by executable: ${PJRT_PLUGIN_FULL_PATH}")
message(STATUS "RPATH to be set for pjrt_loader (helps find dependencies of the plugin):")
foreach(RPATH_DIR ${RPATH_ENTRIES})
    message(STATUS "  ${RPATH_DIR}")
endforeach()

message(STATUS "To run after building (e.g., from a 'build' directory):")
message(STATUS "  cd build")
message(STATUS "  ./pjrt_loader")