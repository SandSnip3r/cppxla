cmake_minimum_required(VERSION 3.10)
project(MNIST_Example)

add_executable(mnist_main mnist_main.cpp)

target_link_libraries(mnist_main PRIVATE pjrt_cpp)

# Pass the plugin path to the C++ code if main.cpp uses this macro
if(DEFINED PJRT_PLUGIN_FULL_PATH_CONFIG AND NOT PJRT_PLUGIN_FULL_PATH_CONFIG STREQUAL "")
    target_compile_definitions(mnist_main PRIVATE 
        "PJRT_PLUGIN_PATH=\"${PJRT_PLUGIN_FULL_PATH_CONFIG}\""
    )
    message(STATUS "Example 'mnist_main' will use PJRT_PLUGIN_PATH: ${PJRT_PLUGIN_FULL_PATH_CONFIG}")
else()
    message(WARNING "PJRT_PLUGIN_FULL_PATH_CONFIG is not defined. 'mnist_main' might not find the PJRT plugin unless the path is provided another way.")
endif()

# Set RPATH for the executable using entries from PJRTSettings.cmake
if(DEFINED RPATH_ENTRIES_CONFIG AND RPATH_ENTRIES_CONFIG)
    set(RPATH_LINKER_FLAGS "")
    foreach(RPATH_DIR ${RPATH_ENTRIES_CONFIG})
        if(IS_ABSOLUTE "${RPATH_DIR}")
            list(APPEND RPATH_LINKER_FLAGS "-Wl,-rpath,${RPATH_DIR}")
        else()
            # This case should ideally be avoided by ensuring absolute paths in PJRTSettings
            list(APPEND RPATH_LINKER_FLAGS "-Wl,-rpath,${CMAKE_SOURCE_DIR}/${RPATH_DIR}")
        endif()
    endforeach()

    if(RPATH_LINKER_FLAGS)
        target_link_options(mnist_main PRIVATE ${RPATH_LINKER_FLAGS})
        message(STATUS "RPATHs for mnist_main: ${RPATH_ENTRIES_CONFIG}")
    endif()
else()
    message(STATUS "No RPATH_ENTRIES_CONFIG defined for mnist_main. Ensure plugin dependencies are findable.")
endif()
