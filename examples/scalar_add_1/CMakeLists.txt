add_executable(scalar_add_1 main.cpp)

target_link_libraries(scalar_add_1 PRIVATE pjrt_cpp ${CMAKE_DL_LIBS})

# Pass the plugin path to the C++ code if main.cpp uses this macro
if(DEFINED PJRT_PLUGIN_FULL_PATH_CONFIG AND NOT PJRT_PLUGIN_FULL_PATH_CONFIG STREQUAL "")
    target_compile_definitions(scalar_add_1 PRIVATE
        "PJRT_PLUGIN_PATH=\"${PJRT_PLUGIN_FULL_PATH_CONFIG}\""
    )
    message(STATUS "Example 'scalar_add_1' will use PJRT_PLUGIN_PATH: ${PJRT_PLUGIN_FULL_PATH_CONFIG}")
else()
    message(WARNING "PJRT_PLUGIN_FULL_PATH_CONFIG is not defined. 'scalar_add_1' might not find the PJRT plugin unless the path is provided another way.")
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
        target_link_options(scalar_add_1 PRIVATE ${RPATH_LINKER_FLAGS})
        message(STATUS "RPATHs for scalar_add_1: ${RPATH_ENTRIES_CONFIG}")
    endif()
else()
    message(STATUS "No RPATH_ENTRIES_CONFIG defined for scalar_add_1. Ensure plugin dependencies are findable.")
endif()

# Copy example data files to the build directory next to the executable if needed
# This makes it easier to run the example from the build directory.
# For myJax.py and myStableHlo.txt
set(EXAMPLE_DATA_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/myJax.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/myStableHlo.txt"
)

foreach(DATA_FILE ${EXAMPLE_DATA_FILES})
    get_filename_component(DATA_FILE_NAME ${DATA_FILE} NAME)
    add_custom_command(
        TARGET scalar_add_1 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DATA_FILE}"
                "$<TARGET_FILE_DIR:scalar_add_1>/${DATA_FILE_NAME}"
        COMMENT "Copying ${DATA_FILE_NAME} to executable directory"
    )
endforeach()

message(STATUS "Configured example: scalar_add_1")
message(STATUS "Run from build directory: cd <build_dir>/examples && ./scalar_add_1")