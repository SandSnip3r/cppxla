add_library(pjrt_cpp STATIC)

target_sources(pjrt_cpp PRIVATE
    buffer.cpp
    buffer.hpp
    client.cpp
    client.hpp
    context.cpp
    context.hpp
    deviceView.cpp
    deviceView.hpp
    event.cpp
    event.hpp
    exception.cpp
    exception.hpp
    executable.cpp
    executable.hpp
    loadedExecutable.cpp
    loadedExecutable.hpp
    detail/callbackUserData.cpp
    detail/callbackUserData.hpp
)

target_include_directories(pjrt_cpp
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

# Define PJRT_PLUGIN_PATH for the library itself
if(DEFINED PJRT_PLUGIN_FULL_PATH_CONFIG AND NOT PJRT_PLUGIN_FULL_PATH_CONFIG STREQUAL "")
    target_compile_definitions(pjrt_cpp PRIVATE
        "PJRT_PLUGIN_PATH=\"${PJRT_PLUGIN_FULL_PATH_CONFIG}\""
    )
    message(STATUS "Library 'pjrt_cpp' will be compiled with PJRT_PLUGIN_PATH: ${PJRT_PLUGIN_FULL_PATH_CONFIG}")
else()
    message(WARNING "PJRT_PLUGIN_FULL_PATH_CONFIG is not defined for pjrt_cpp library. Compilation of context.cpp will likely fail due to missing PJRT_PLUGIN_PATH macro.")
endif()

message(STATUS "Configured pjrt_cpp library with explicitly listed sources (in pjrt/, no installation rules).")