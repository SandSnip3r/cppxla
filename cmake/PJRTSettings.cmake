# This module detects the Python .venv environment and sets up paths
# for the PJRT plugin and necessary RPATHs for executables.
#
# It defines the following variables in the parent scope:
# - PJRT_PLUGIN_FULL_PATH_CONFIG: Absolute path to the PJRT plugin .so file.
# - RPATH_ENTRIES_CONFIG: A CMake list of directories to be added to RPATH.

message(STATUS "PJRTSettings.cmake: Detecting Python .venv and PJRT plugin paths...")

# Define CACHE variables once at the beginning
set(PJRT_PLUGIN_FULL_PATH_CONFIG "" CACHE STRING "Full path to the PJRT plugin shared library (e.g., xla_cuda_plugin.so)")
set(RPATH_ENTRIES_CONFIG "" CACHE STRING "Semicolon-separated list of RPATH entries for executables")

# --- Detect .venv Python specific paths ---
file(GLOB VENV_PYTHON_LIB_CANDIDATES "${CMAKE_SOURCE_DIR}/.venv/lib/python3.*")

set(VENV_PYTHON_LIB_DIR "")
if(VENV_PYTHON_LIB_CANDIDATES)
    list(GET VENV_PYTHON_LIB_CANDIDATES 0 VENV_PYTHON_LIB_DIR)
    if(NOT IS_DIRECTORY ${VENV_PYTHON_LIB_DIR})
        message(WARNING "Detected venv Python lib directory is not a directory: ${VENV_PYTHON_LIB_DIR}. PJRT plugin detection may fail.")
        set(VENV_PYTHON_LIB_DIR "") # Reset if not valid
    else()
        message(STATUS "PJRTSettings: Detected venv Python library directory: ${VENV_PYTHON_LIB_DIR}")
    endif()
else()
    message(WARNING "PJRTSettings: Could not find a python3.x directory in .venv/lib/. Please ensure '.venv' is created and 'jax[cuda]' is installed if you intend to run examples/tests with a local JAX install.")
endif()

# --- Initialize internal variables ---
set(PJRT_PLUGIN_FULL_PATH_CONFIG_INTERNAL "")
set(RPATH_ENTRIES_CONFIG_INTERNAL "")

if(VENV_PYTHON_LIB_DIR)
    set(VENV_SITE_PACKAGES_DIR "${VENV_PYTHON_LIB_DIR}/site-packages")
    if(NOT IS_DIRECTORY ${VENV_SITE_PACKAGES_DIR})
        message(WARNING "PJRTSettings: Venv site-packages directory not found: ${VENV_SITE_PACKAGES_DIR}")
    else()
        # --- Define PJRT Plugin Path & RPATHs based on .venv ---
        # Try to find common plugin names, adjust if necessary (e.g. xla_cpu_plugin.so)
        find_file(PJRT_PLUGIN_SO_PATH_DETECTED # Use a distinct name for the find_file result
            NAMES xla_cuda_plugin.so xla_cpu_plugin.so libtpu.so
            PATHS "${VENV_SITE_PACKAGES_DIR}/jax_plugins/xla_cuda12" # Common for CUDA 12
                  "${VENV_SITE_PACKAGES_DIR}/jax_plugins/xla_cuda11" # Common for CUDA 11
                  "${VENV_SITE_PACKAGES_DIR}/jax_plugins"
                  "${VENV_SITE_PACKAGES_DIR}/google/cloud/tpu/lib" # For TPU
                  "${VENV_SITE_PACKAGES_DIR}" # Fallback
            NO_DEFAULT_PATH
        )

        if(EXISTS "${PJRT_PLUGIN_SO_PATH_DETECTED}")
            get_filename_component(PJRT_PLUGIN_DIR_DETECTED "${PJRT_PLUGIN_SO_PATH_DETECTED}" DIRECTORY)
            set(PJRT_PLUGIN_FULL_PATH_CONFIG_INTERNAL "${PJRT_PLUGIN_SO_PATH_DETECTED}") # Store in internal var
            message(STATUS "PJRTSettings: Found PJRT Plugin: ${PJRT_PLUGIN_FULL_PATH_CONFIG_INTERNAL}")

            set(CUDNN_LIB_DIR_CANDIDATE "${VENV_SITE_PACKAGES_DIR}/nvidia/cudnn/lib")
            set(CUDA_RUNTIME_LIB_DIR_CANDIDATE "${VENV_SITE_PACKAGES_DIR}/nvidia/cuda_runtime/lib")
            set(JAXLIB_DIR_CANDIDATE "${VENV_SITE_PACKAGES_DIR}/jaxlib") # For libjax_common.so etc.

            # RPATH_ENTRIES_CONFIG_INTERNAL is already initialized as an empty list
            list(APPEND RPATH_ENTRIES_CONFIG_INTERNAL "${PJRT_PLUGIN_DIR_DETECTED}")

            if(IS_DIRECTORY ${CUDNN_LIB_DIR_CANDIDATE})
                list(APPEND RPATH_ENTRIES_CONFIG_INTERNAL "${CUDNN_LIB_DIR_CANDIDATE}")
            else()
                message(STATUS "PJRTSettings: Optional CUDNN lib directory for RPATH not found in .venv: ${CUDNN_LIB_DIR_CANDIDATE}")
            endif()
            if(IS_DIRECTORY ${CUDA_RUNTIME_LIB_DIR_CANDIDATE})
                list(APPEND RPATH_ENTRIES_CONFIG_INTERNAL "${CUDA_RUNTIME_LIB_DIR_CANDIDATE}")
            else()
                message(STATUS "PJRTSettings: Optional CUDA Runtime lib directory for RPATH not found in .venv: ${CUDA_RUNTIME_LIB_DIR_CANDIDATE}")
            endif()
            if(IS_DIRECTORY ${JAXLIB_DIR_CANDIDATE})
                list(APPEND RPATH_ENTRIES_CONFIG_INTERNAL "${JAXLIB_DIR_CANDIDATE}")
            else()
                message(STATUS "PJRTSettings: Optional JAXLIB directory for RPATH not found in .venv: ${JAXLIB_DIR_CANDIDATE}")
            endif()
            # The RPATH_ENTRIES_CONFIG_INTERNAL list is now populated
        else()
            message(WARNING "PJRTSettings: PJRT Plugin (e.g., xla_cuda_plugin.so) not found in expected .venv paths. Examples/tests requiring a plugin might fail or need manual PJRT_PLUGIN_FULL_PATH_CONFIG.")
        endif()
    endif()
else()
    message(STATUS "PJRTSettings: .venv not detected or invalid. Skipping automatic PJRT plugin path configuration from .venv.")
    message(STATUS "PJRTSettings: You may need to set PJRT_PLUGIN_FULL_PATH_CONFIG manually if building executables that load a PJRT plugin.")
endif()

# Ensure CMAKE_MACOSX_RPATH is handled if on macOS, typically ON by default for modern CMake.
# For Linux, RPATH is typically handled via linker flags.
set(CMAKE_MACOSX_RPATH ON) # Explicitly set for clarity, usually default

# --- Finalize and set PJRT_PLUGIN_FULL_PATH_CONFIG ---
if(PJRT_PLUGIN_FULL_PATH_CONFIG_INTERNAL) # Check if the internal variable has a value
    set(PJRT_PLUGIN_FULL_PATH_CONFIG "${PJRT_PLUGIN_FULL_PATH_CONFIG_INTERNAL}")
else()
    set(PJRT_PLUGIN_FULL_PATH_CONFIG "") # Ensure it's empty if not found
endif()
# Update the CACHE entry with the determined value (or empty if not found)
set(PJRT_PLUGIN_FULL_PATH_CONFIG "${PJRT_PLUGIN_FULL_PATH_CONFIG}" CACHE STRING "Full path to the PJRT plugin shared library" FORCE)

# --- Finalize and set RPATH_ENTRIES_CONFIG ---
if(RPATH_ENTRIES_CONFIG_INTERNAL) # Check if the internal list has any entries
    set(RPATH_ENTRIES_CONFIG ${RPATH_ENTRIES_CONFIG_INTERNAL})
else()
    set(RPATH_ENTRIES_CONFIG ) # Ensure it's an empty list if not populated
endif()
# Update the CACHE entry with the determined list (or empty if not populated)
set(RPATH_ENTRIES_CONFIG "${RPATH_ENTRIES_CONFIG}" CACHE STRING "Semicolon-separated list of RPATH entries for executables" FORCE)


message(STATUS "PJRTSettings: PJRT_PLUGIN_FULL_PATH_CONFIG ultimately set to: ${PJRT_PLUGIN_FULL_PATH_CONFIG}")
message(STATUS "PJRTSettings: RPATH_ENTRIES_CONFIG ultimately set to: ${RPATH_ENTRIES_CONFIG}")