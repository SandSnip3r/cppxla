cmake_minimum_required(VERSION 3.14)

# 1. Fetch GoogleTest using FetchContent
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
# For GTest 1.10+ and Windows MSVC, this can be helpful:
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 2. Define your test executable
add_executable(pjrt_lib_tests
    test_initialization.cpp
    # Add other test_*.cpp files here
)

# 3. Link your library, DL libs, and GTest
# Linking against GTest::gtest should automatically add its include directories.
target_link_libraries(pjrt_lib_tests PRIVATE
    pjrt_cpp
    ${CMAKE_DL_LIBS}  # For dlopen used by PJRT
    GTest::gtest
    GTest::gtest_main
)

# 4. Pass the PJRT plugin path (from PJRTSettings.cmake)
if(DEFINED PJRT_PLUGIN_FULL_PATH_CONFIG AND NOT PJRT_PLUGIN_FULL_PATH_CONFIG STREQUAL "")
    target_compile_definitions(pjrt_lib_tests PRIVATE
        "PJRT_PLUGIN_PATH=\"${PJRT_PLUGIN_FULL_PATH_CONFIG}\""
    )
    message(STATUS "Tests 'pjrt_lib_tests' will use PJRT_PLUGIN_PATH: ${PJRT_PLUGIN_FULL_PATH_CONFIG}")
else()
    message(WARNING "PJRT_PLUGIN_FULL_PATH_CONFIG is not defined for pjrt_lib_tests. Tests might not compile or run correctly.")
endif()

# 5. Set RPATH for the test executable (important for runtime loading)
if(DEFINED RPATH_ENTRIES_CONFIG AND RPATH_ENTRIES_CONFIG)
    set(RPATH_LINKER_FLAGS_TEST "")
    foreach(RPATH_DIR ${RPATH_ENTRIES_CONFIG})
        if(IS_ABSOLUTE "${RPATH_DIR}")
            list(APPEND RPATH_LINKER_FLAGS_TEST "-Wl,-rpath,${RPATH_DIR}")
        else()
            # This path should be absolute if coming from PJRTSettings.cmake
            list(APPEND RPATH_LINKER_FLAGS_TEST "-Wl,-rpath,${CMAKE_SOURCE_DIR}/${RPATH_DIR}")
        endif()
    endforeach()

    if(RPATH_LINKER_FLAGS_TEST)
        target_link_options(pjrt_lib_tests PRIVATE ${RPATH_LINKER_FLAGS_TEST})
        message(STATUS "RPATHs for pjrt_lib_tests: ${RPATH_ENTRIES_CONFIG}")
    endif()
else()
    message(STATUS "No RPATH_ENTRIES_CONFIG defined for pjrt_lib_tests. Ensure plugin dependencies are findable.")
endif()

# 6. Add the test to CTest for discoverability
include(GoogleTest)
if(TARGET pjrt_lib_tests)
    gtest_discover_tests(pjrt_lib_tests)
endif()

message(STATUS "Configured GTest tests: pjrt_lib_tests")
message(STATUS "Run tests from build directory: cd <build_dir> && ctest")