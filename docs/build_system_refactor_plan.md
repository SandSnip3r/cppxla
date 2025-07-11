# Build System Refactor Plan: Creating Portable Binaries

## 1. Goal

The primary goal of this refactor is to change the project's build system to produce **portable (relocatable) binaries**.

Currently, our build process creates executables (like `mnist_main`) that only work on the machine they were built on, and only if the project directory is not moved. This is because the binaries have hardcoded, absolute paths to shared library dependencies located inside the local `.venv` directory.

This refactor will enable developers and users to copy the build output to other machines or locations and run the executables without recompilation.

## 2. The Problem in Detail

There are two sources of hardcoded paths:

1.  **Runtime Plugin Loading (`dlopen`)**: The `pjrt::Context` constructor loads the PJRT plugin (e.g., `xla_cuda_plugin.so`) using a path that is "baked in" at compile time via a C++ macro (`-DPJRT_PLUGIN_PATH=...`). If the `.so` file is not at that exact absolute path during execution, the program fails.

2.  **Dynamic Linker (`RPATH`)**: The PJRT plugin itself depends on other shared libraries, such as NVIDIA's CUDA and cuDNN libraries, which are also in the `.venv`. To ensure the dynamic linker can find these, the build system embeds an `RPATH` into our executables. This `RPATH` contains absolute paths to the directories containing these CUDA libraries. If these directories don't exist at runtime, the program will fail to even start.

## 3. Proposed Solution

We will adopt a standard industry approach for creating portable Linux applications. This involves two main changes:

1.  **Make the Executable Relocatable**: We will change the `RPATH` from an absolute path to a **relative path**. We will use the special `$ORIGIN` variable, which tells the linker to look for libraries relative to the executable's own location. We will configure the `RPATH` to point to a `lib/` directory that is a sibling to the `bin/` directory containing our executable (e.g., `../lib`).

2.  **Bundle Dependencies**: We will modify the CMake build process to **copy** all required shared libraries (the PJRT plugin, CUDA/cuDNN libraries, etc.) from the `.venv` into our new `build/lib` directory. This process is often called "bundling".

3.  **Flexible Plugin Loading**: We will modify `pjrt/context.cpp` to be more flexible. It will prioritize finding the PJRT plugin via a `PJRT_PLUGIN_PATH` environment variable. If the variable is not set, it will fall back to the compile-time path, ensuring the "out-of-the-box" developer experience still works.

### Resulting Build Directory Structure

After these changes, the `build` directory will be self-contained and look like this:

```
build/
├── bin/
│   └── mnist_main
└── lib/
    ├── xla_cuda_plugin.so
    ├── libcudart.so.12
    ├── libcudnn.so.8
    └── ... (and all other dependencies)
```

## 4. The New User Experience

This refactor will significantly improve the experience for anyone using this library.

**Current (Bad) Experience:**
A developer clones, builds, and runs the code. It works. They then `mv` the project directory to a new location. The program now fails with a `dlopen` error. Or, they send the `mnist_main` binary to a colleague, who gets a "file not found" error from the dynamic linker because the `RPATH` is pointing to the original developer's home directory.

**New (Good) Experience:**
A developer clones, builds, and runs the code. The build process automatically copies the dependencies into `build/lib`. The `mnist_main` executable in `build/bin` works immediately.

They can then create a `.tar.gz` of the `build` directory and send it to a colleague. The colleague can extract it anywhere on their machine and `./build/bin/mnist_main` will run successfully, as all its dependencies are right there in the adjacent `lib/` folder.

A power user can still override this behavior by setting the `PJRT_PLUGIN_PATH` and `LD_LIBRARY_PATH` environment variables to point to custom library locations.

## 5. Step-by-Step Implementation Guide

### Step 1: Modify `pjrt/context.cpp` for Hybrid Plugin Loading

Update the `Context` constructor to prioritize an environment variable for the plugin path.

**File:** `pjrt/context.cpp`

```cpp
// Add these includes at the top
#include <cstdlib> // Required for std::getenv
#include <iostream>
#include <stdexcept>

// ...

namespace pjrt {

// Add this helper function
const char* GetPjrtPluginPath() {
    // 1. Try getting from environment variable first
    const char* plugin_path_env = std::getenv("PJRT_PLUGIN_PATH");
    if (plugin_path_env != nullptr && plugin_path_env[0] != '\0') {
        std::cout << "Using PJRT plugin path from environment variable: " << plugin_path_env << std::endl;
        return plugin_path_env;
    }

    // 2. Fallback to compile-time definition
    #ifdef PJRT_PLUGIN_PATH
        std::cout << "Using PJRT plugin path from compile-time definition: " << PJRT_PLUGIN_PATH << std::endl;
        return PJRT_PLUGIN_PATH;
    #else
        return nullptr; // No path available
    #endif
}

// Modify the constructor
Context::Context() {
  const char* pluginPath = GetPjrtPluginPath();
  if (pluginPath == nullptr) {
    throw std::runtime_error("PJRT_PLUGIN_PATH is not set. Please set the environment variable "
                             "or define it at compile time via CMake.");
  }
  std::cout << "PJRT Plugin Path: \"" << pluginPath << "\"" << std::endl;

  // ... rest of the constructor
```

### Step 2: Update `PJRTSettings.cmake` to Copy Dependencies

Modify the main CMake settings file to copy all found libraries into the build output directory.

**File:** `cmake/PJRTSettings.cmake`

```cmake
# ... (at the end of the file, after RPATH_ENTRIES_CONFIG is finalized) ...

# --- Bundle Dependencies into the Build Directory ---
set(BUNDLE_LIB_DIR ${CMAKE_BINARY_DIR}/lib)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release) # Default to release if not set
endif()

message(STATUS "Bundling dependencies to: ${BUNDLE_LIB_DIR}")

# Ensure the library directory exists
file(MAKE_DIRECTORY ${BUNDLE_LIB_DIR})

# Copy all the directories found for the RPATH into our bundle lib dir
if(RPATH_ENTRIES_CONFIG)
    foreach(RPATH_DIR ${RPATH_ENTRIES_CONFIG})
        if(IS_DIRECTORY ${RPATH_DIR})
            message(STATUS "Copying libraries from ${RPATH_DIR} to ${BUNDLE_LIB_DIR}")
            file(GLOB SHARED_LIBS "${RPATH_DIR}/*.so*")
            if(SHARED_LIBS)
                file(COPY ${SHARED_LIBS} DESTINATION ${BUNDLE_LIB_DIR})
            else()
                message(WARNING "No .so files found in ${RPATH_DIR}")
            endif()
        endif()
    endforeach()
endif()
```

### Step 3: Update Executable CMakeLists to Use Relative RPATH

Modify the `CMakeLists.txt` for any executable target to set a relative `RPATH`.

**File:** `examples/mnist/CMakeLists.txt`

```cmake
# ... after add_executable() and target_link_libraries() ...

# Set RPATH for the executable to be relative to its location
# This makes the binary portable.
set_target_properties(mnist_main PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE,
    INSTALL_RPATH "$ORIGIN/../lib"
)

# The target_link_options from the old file can be removed.
# The block starting with "if(DEFINED RPATH_ENTRIES_CONFIG...)" should be deleted.
```

After applying these changes, run a clean build (`rm -rf build && cmake -B build . && cmake --build build`). You will see the new `build/lib` directory populated with the necessary libraries, and the `mnist_main` executable will be fully portable.
