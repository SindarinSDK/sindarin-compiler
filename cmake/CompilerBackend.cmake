# CompilerBackend.cmake - Backend selection and configuration for Sn Compiler
#
# This module handles the selection and configuration of C compiler backends
# (GCC, Clang, TinyCC) for compiling generated C code from Sindarin sources.
#
# Variables:
#   SN_BACKEND - The backend to use (gcc, clang, tcc). Auto-detected if not set.
#   SN_BACKEND_DIR - The directory for backend-specific runtime objects
#
# Functions:
#   sn_detect_backend() - Auto-detect the best available backend
#   sn_configure_backend() - Configure the backend for use

include_guard(GLOBAL)

# Backend option (can be set via preset or command line)
set(SN_BACKEND "" CACHE STRING "C compiler backend for generated code (gcc, clang, tcc)")
set_property(CACHE SN_BACKEND PROPERTY STRINGS "" "gcc" "clang" "tcc")

# Detect the best available backend based on platform and available compilers
function(sn_detect_backend)
    if(SN_BACKEND)
        message(STATUS "Using specified backend: ${SN_BACKEND}")
        return()
    endif()

    # Platform-specific defaults
    if(WIN32)
        # Windows: prefer Clang (LLVM-MinGW)
        find_program(CLANG_FOUND clang)
        if(CLANG_FOUND)
            set(SN_BACKEND "clang" CACHE STRING "" FORCE)
            message(STATUS "Auto-detected backend: clang (Windows default)")
            return()
        endif()
    elseif(APPLE)
        # macOS: prefer Apple Clang
        set(SN_BACKEND "clang" CACHE STRING "" FORCE)
        message(STATUS "Auto-detected backend: clang (macOS default)")
        return()
    else()
        # Linux: prefer GCC
        find_program(GCC_FOUND gcc)
        if(GCC_FOUND)
            set(SN_BACKEND "gcc" CACHE STRING "" FORCE)
            message(STATUS "Auto-detected backend: gcc (Linux default)")
            return()
        endif()

        find_program(CLANG_FOUND clang)
        if(CLANG_FOUND)
            set(SN_BACKEND "clang" CACHE STRING "" FORCE)
            message(STATUS "Auto-detected backend: clang")
            return()
        endif()
    endif()

    # Fallback to whatever we're building with
    if(CMAKE_C_COMPILER_ID MATCHES "GNU")
        set(SN_BACKEND "gcc" CACHE STRING "" FORCE)
    elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
        set(SN_BACKEND "clang" CACHE STRING "" FORCE)
    else()
        set(SN_BACKEND "gcc" CACHE STRING "" FORCE)
    endif()
    message(STATUS "Auto-detected backend: ${SN_BACKEND} (from build compiler)")
endfunction()

# Configure the backend directory and settings
function(sn_configure_backend)
    sn_detect_backend()

    # Set the runtime library directory based on backend
    # This is where pre-compiled runtime .o files go
    if(SN_BACKEND STREQUAL "gcc")
        set(SN_BACKEND_DIR "${CMAKE_SOURCE_DIR}/bin/lib/gcc" PARENT_SCOPE)
        set(SN_RUNTIME_COMPILER "gcc" PARENT_SCOPE)
    elseif(SN_BACKEND STREQUAL "clang")
        set(SN_BACKEND_DIR "${CMAKE_SOURCE_DIR}/bin/lib/clang" PARENT_SCOPE)
        set(SN_RUNTIME_COMPILER "clang" PARENT_SCOPE)
    elseif(SN_BACKEND STREQUAL "tcc")
        set(SN_BACKEND_DIR "${CMAKE_SOURCE_DIR}/bin/lib/tinycc" PARENT_SCOPE)
        set(SN_RUNTIME_COMPILER "tcc" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Unknown backend: ${SN_BACKEND}")
    endif()

    message(STATUS "Backend: ${SN_BACKEND}")
    message(STATUS "Runtime directory: ${SN_BACKEND_DIR}")
endfunction()

# Get backend-specific compiler flags for runtime compilation
function(sn_get_backend_flags OUT_VAR)
    set(FLAGS "-O2 -std=c99")

    if(SN_BACKEND STREQUAL "gcc")
        list(APPEND FLAGS "-D_GNU_SOURCE")
    elseif(SN_BACKEND STREQUAL "clang")
        list(APPEND FLAGS "-D_GNU_SOURCE")
    elseif(SN_BACKEND STREQUAL "tcc")
        # TinyCC has limited optimization options
        set(FLAGS "-std=c99")
    endif()

    # Platform-specific flags
    if(WIN32)
        list(APPEND FLAGS "-D_WIN32")
    endif()

    string(REPLACE ";" " " FLAGS_STR "${FLAGS}")
    set(${OUT_VAR} "${FLAGS_STR}" PARENT_SCOPE)
endfunction()
