# RuntimeObjects.cmake - Build runtime object files for Sn Compiler
#
# This module handles the compilation of runtime source files into individual
# object files that get linked with generated C code from Sindarin programs.
#
# Functions:
#   sn_add_runtime_object() - Add a single runtime object file
#   sn_add_all_runtime_objects() - Add all runtime objects
#   sn_setup_runtime_headers() - Copy runtime headers to bin/include

include_guard(GLOBAL)

# List of all runtime source files
set(SN_RUNTIME_SOURCE_FILES
    arena.c
    debug.c
    runtime.c
    runtime/runtime_arena.c
    runtime/runtime_string.c
    runtime/runtime_array.c
    runtime/runtime_text_file.c
    runtime/runtime_binary_file.c
    runtime/runtime_io.c
    runtime/runtime_byte.c
    runtime/runtime_path.c
    runtime/runtime_thread.c
    runtime/runtime_env.c
    runtime/runtime_any.c
    runtime/runtime_intercept.c
)

# Add a single runtime object compilation command
# Arguments:
#   SOURCE_FILE - Relative path from src/ (e.g., "runtime.c" or "runtime/runtime_string.c")
#   OUTPUT_NAME - Name of output object file without extension (e.g., "runtime" or "runtime_string")
function(sn_add_runtime_object SOURCE_FILE OUTPUT_NAME)
    set(OUTPUT_FILE "${SN_BACKEND_DIR}/${OUTPUT_NAME}.o")
    set(SOURCE_PATH "${CMAKE_SOURCE_DIR}/src/${SOURCE_FILE}")

    # Get the compiler to use for runtime
    if(NOT DEFINED SN_RUNTIME_COMPILER)
        set(SN_RUNTIME_COMPILER "${CMAKE_C_COMPILER}")
    endif()

    # Include directories
    set(INCLUDE_FLAGS
        -I${CMAKE_SOURCE_DIR}/src
        -I${CMAKE_SOURCE_DIR}/src/runtime
        -I${CMAKE_SOURCE_DIR}/src/platform
    )

    # Platform-specific definitions
    if(NOT WIN32)
        list(APPEND INCLUDE_FLAGS -D_GNU_SOURCE)
    endif()

    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SN_BACKEND_DIR}
        COMMAND ${SN_RUNTIME_COMPILER} -c ${SOURCE_PATH} -o ${OUTPUT_FILE} ${INCLUDE_FLAGS} -O2 -std=c99
        DEPENDS ${SOURCE_PATH}
        COMMENT "Compiling runtime: ${OUTPUT_NAME}.o"
        VERBATIM
    )

    # Add to global list of runtime objects
    set(SN_RUNTIME_OBJECTS ${SN_RUNTIME_OBJECTS} ${OUTPUT_FILE} PARENT_SCOPE)
endfunction()

# Add all runtime object files
function(sn_add_all_runtime_objects)
    set(LOCAL_RUNTIME_OBJECTS "")

    foreach(SOURCE_FILE ${SN_RUNTIME_SOURCE_FILES})
        # Extract the output name from the source file
        get_filename_component(FILE_NAME ${SOURCE_FILE} NAME_WE)

        set(OUTPUT_FILE "${SN_BACKEND_DIR}/${FILE_NAME}.o")
        set(SOURCE_PATH "${CMAKE_SOURCE_DIR}/src/${SOURCE_FILE}")

        # Include directories
        set(INCLUDE_FLAGS
            -I${CMAKE_SOURCE_DIR}/src
            -I${CMAKE_SOURCE_DIR}/src/runtime
            -I${CMAKE_SOURCE_DIR}/src/platform
        )

        # Platform-specific definitions
        if(NOT WIN32)
            list(APPEND INCLUDE_FLAGS -D_GNU_SOURCE)
        endif()

        add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SN_BACKEND_DIR}
            COMMAND ${CMAKE_C_COMPILER} -c ${SOURCE_PATH} -o ${OUTPUT_FILE} ${INCLUDE_FLAGS} -O2 -std=c99
            DEPENDS ${SOURCE_PATH}
            COMMENT "Compiling runtime: ${FILE_NAME}.o"
            VERBATIM
        )

        list(APPEND LOCAL_RUNTIME_OBJECTS ${OUTPUT_FILE})
    endforeach()

    # Create a target that depends on all runtime objects
    add_custom_target(sn_runtime ALL DEPENDS ${LOCAL_RUNTIME_OBJECTS})

    # Export the list for use elsewhere
    set(SN_RUNTIME_OBJECTS ${LOCAL_RUNTIME_OBJECTS} PARENT_SCOPE)
endfunction()

# Setup runtime headers in bin/include
function(sn_setup_runtime_headers)
    set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/bin/include")
    set(RUNTIME_INCLUDE_DIR "${INCLUDE_DIR}/runtime")
    set(PLATFORM_INCLUDE_DIR "${INCLUDE_DIR}/platform")

    # Create directories
    file(MAKE_DIRECTORY ${INCLUDE_DIR})
    file(MAKE_DIRECTORY ${RUNTIME_INCLUDE_DIR})
    file(MAKE_DIRECTORY ${PLATFORM_INCLUDE_DIR})

    # Copy main runtime header
    if(EXISTS "${CMAKE_SOURCE_DIR}/src/runtime.h")
        file(INSTALL "${CMAKE_SOURCE_DIR}/src/runtime.h" DESTINATION "${INCLUDE_DIR}")
    endif()

    # Copy runtime subdirectory headers
    file(GLOB RUNTIME_HEADERS "${CMAKE_SOURCE_DIR}/src/runtime/*.h")
    if(RUNTIME_HEADERS)
        file(INSTALL ${RUNTIME_HEADERS} DESTINATION "${RUNTIME_INCLUDE_DIR}")
    endif()

    # Copy platform headers
    file(GLOB PLATFORM_HEADERS "${CMAKE_SOURCE_DIR}/src/platform/*.h")
    if(PLATFORM_HEADERS)
        file(INSTALL ${PLATFORM_HEADERS} DESTINATION "${PLATFORM_INCLUDE_DIR}")
    endif()

    message(STATUS "Runtime headers installed to: ${INCLUDE_DIR}")
endfunction()

# Setup config files in bin/
function(sn_setup_config_files)
    file(GLOB CONFIG_FILES "${CMAKE_SOURCE_DIR}/etc/*.cfg")
    foreach(CONFIG_FILE ${CONFIG_FILES})
        get_filename_component(CONFIG_NAME ${CONFIG_FILE} NAME)
        set(DEST_CONFIG "${CMAKE_SOURCE_DIR}/bin/${CONFIG_NAME}")
        if(NOT EXISTS ${DEST_CONFIG})
            file(INSTALL ${CONFIG_FILE} DESTINATION "${CMAKE_SOURCE_DIR}/bin")
        endif()
    endforeach()
endfunction()
