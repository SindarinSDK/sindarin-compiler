# Sn Compiler - CMake Build System
# Cross-platform build for Linux, macOS, and Windows
#
# Quick Start:
#   cmake --preset linux-gcc-release    # Configure for Linux with GCC
#   cmake --build --preset linux-gcc-release  # Build
#
# See CMakePresets.json for all available presets.

cmake_minimum_required(VERSION 3.16)

# Version can be overridden via -DSN_VERSION=x.y.z (used by CI for tag-based versioning)
if(NOT DEFINED SN_VERSION)
    set(SN_VERSION "1.0.0")
endif()

project(sn VERSION ${SN_VERSION} LANGUAGES C)

# Add custom cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Use C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Output directories - all artifacts go to bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

#------------------------------------------------------------------------------
# Build Options
#------------------------------------------------------------------------------
option(SN_DEBUG "Build with debug symbols and sanitizers" OFF)
option(SN_ASAN "Enable AddressSanitizer (GCC/Clang only)" OFF)
set(SN_BACKEND "" CACHE STRING "C compiler backend (gcc, clang, tcc)")

#------------------------------------------------------------------------------
# Compiler Flags
#------------------------------------------------------------------------------
# Common warnings for all compilers
add_compile_options(-Wall -Wextra)

# Suppress warnings for platform-specific pragmas
add_compile_options(-Wno-unknown-pragmas)

# Clang-specific: suppress typedef redefinition warnings (valid C99)
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-typedef-redefinition)
endif()

# Define _GNU_SOURCE for Linux extensions
add_compile_definitions(_GNU_SOURCE)

# Debug vs Release configuration
if(SN_DEBUG)
    add_compile_options(-g -O0)
    if(SN_ASAN)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
else()
    add_compile_options(-O3 -flto)
    add_link_options(-flto -O3)
endif()

#------------------------------------------------------------------------------
# Source Files - Organized by Module
#------------------------------------------------------------------------------
set(SN_CORE_SOURCES
    src/arena.c
    src/file.c
    src/token.c
    src/debug.c
    src/diagnostic.c
    src/compiler.c
)

set(SN_LEXER_SOURCES
    src/lexer.c
    src/lexer/lexer_util.c
    src/lexer/lexer_scan.c
)

set(SN_AST_SOURCES
    src/ast.c
    src/ast/ast_type.c
    src/ast/ast_expr.c
    src/ast/ast_stmt.c
    src/ast/ast_print.c
)

set(SN_PARSER_SOURCES
    src/parser.c
    src/parser/parser_util.c
    src/parser/parser_expr.c
    src/parser/parser_stmt.c
    src/parser/parser_stmt_decl.c
    src/parser/parser_stmt_control.c
)

set(SN_SYMBOL_TABLE_SOURCES
    src/symbol_table.c
    src/symbol_table/symbol_table_core.c
    src/symbol_table/symbol_table_namespace.c
    src/symbol_table/symbol_table_thread.c
)

set(SN_TYPE_CHECKER_SOURCES
    src/type_checker.c
    src/type_checker/type_checker_util.c
    src/type_checker/type_checker_expr.c
    src/type_checker/type_checker_expr_call.c
    src/type_checker/type_checker_expr_call_core.c
    src/type_checker/type_checker_expr_call_array.c
    src/type_checker/type_checker_expr_call_string.c
    src/type_checker/type_checker_expr_array.c
    src/type_checker/type_checker_expr_lambda.c
    src/type_checker/type_checker_stmt.c
)

set(SN_OPTIMIZER_SOURCES
    src/optimizer.c
    src/optimizer/optimizer_util.c
    src/optimizer/optimizer_tail_call.c
    src/optimizer/optimizer_string.c
)

set(SN_CODEGEN_SOURCES
    src/code_gen.c
    src/code_gen/code_gen_util.c
    src/code_gen/code_gen_expr.c
    src/code_gen/code_gen_expr_core.c
    src/code_gen/code_gen_expr_binary.c
    src/code_gen/code_gen_expr_lambda.c
    src/code_gen/code_gen_expr_call.c
    src/code_gen/code_gen_expr_call_array.c
    src/code_gen/code_gen_expr_call_string.c
    src/code_gen/code_gen_expr_array.c
    src/code_gen/code_gen_expr_static.c
    src/code_gen/code_gen_expr_string.c
    src/code_gen/code_gen_expr_thread.c
    src/code_gen/code_gen_stmt_core.c
    src/code_gen/code_gen_stmt_loop.c
    src/code_gen/code_gen_stmt_capture.c
)

set(SN_BACKEND_SOURCES
    src/gcc_backend.c
)

set(SN_RUNTIME_SOURCES
    src/runtime.c
    src/runtime/runtime_arena.c
    src/runtime/runtime_string.c
    src/runtime/runtime_array.c
    src/runtime/runtime_io.c
    src/runtime/runtime_byte.c
    src/runtime/runtime_thread.c
    src/runtime/runtime_any.c
    src/runtime/runtime_intercept.c
)

# All compiler sources (excluding main.c)
set(SN_COMPILER_SOURCES
    ${SN_CORE_SOURCES}
    ${SN_LEXER_SOURCES}
    ${SN_AST_SOURCES}
    ${SN_PARSER_SOURCES}
    ${SN_SYMBOL_TABLE_SOURCES}
    ${SN_TYPE_CHECKER_SOURCES}
    ${SN_OPTIMIZER_SOURCES}
    ${SN_CODEGEN_SOURCES}
    ${SN_BACKEND_SOURCES}
    ${SN_RUNTIME_SOURCES}
)

#------------------------------------------------------------------------------
# Main Compiler Executable
#------------------------------------------------------------------------------
add_executable(sn
    src/main.c
    ${SN_COMPILER_SOURCES}
)

target_include_directories(sn PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/runtime
    ${CMAKE_SOURCE_DIR}/src/platform
)

# Add compile-time path defines for installed packages
# These are used as fallback paths when the compiler can't find files relative to exe
if(CMAKE_INSTALL_PREFIX)
    target_compile_definitions(sn PRIVATE
        SN_DEFAULT_CONFIG_FILE="${CMAKE_INSTALL_FULL_SYSCONFDIR}/sindarin/sn.cfg"
        SN_DEFAULT_LIB_DIR="${CMAKE_INSTALL_FULL_LIBDIR}/sindarin"
        SN_DEFAULT_INCLUDE_DIR="${CMAKE_INSTALL_FULL_INCLUDEDIR}/sindarin"
        SN_DEFAULT_SDK_DIR="${CMAKE_INSTALL_FULL_DATADIR}/sindarin/sdk"
    )
endif()

# Platform-specific libraries
if(WIN32)
    target_link_libraries(sn PRIVATE ws2_32 bcrypt pthread)
else()
    target_link_libraries(sn PRIVATE pthread m)
endif()

#------------------------------------------------------------------------------
# Unit Test Executable
#------------------------------------------------------------------------------
add_executable(tests
    tests/unit/_all_.c
    ${SN_COMPILER_SOURCES}
)

target_include_directories(tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/runtime
    ${CMAKE_SOURCE_DIR}/src/platform
)

if(WIN32)
    target_link_libraries(tests PRIVATE ws2_32 bcrypt pthread)
else()
    target_link_libraries(tests PRIVATE pthread m)
endif()

#------------------------------------------------------------------------------
# Runtime Static Library
#------------------------------------------------------------------------------
# Build a single static library instead of individual .o files
# This simplifies linking and avoids command-line length issues on Windows
if(NOT MSVC)
    if(WIN32)
        set(SN_RUNTIME_LIB_DIR ${CMAKE_SOURCE_DIR}/bin/lib/clang)
    else()
        set(SN_RUNTIME_LIB_DIR ${CMAKE_SOURCE_DIR}/bin/lib/gcc)
    endif()

    file(MAKE_DIRECTORY ${SN_RUNTIME_LIB_DIR})

    # Runtime source files
    set(SN_RUNTIME_LIB_SOURCES
        ${CMAKE_SOURCE_DIR}/src/arena.c
        ${CMAKE_SOURCE_DIR}/src/debug.c
        ${CMAKE_SOURCE_DIR}/src/runtime.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_arena.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_string.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_array.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_io.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_byte.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_thread.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_any.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_intercept.c
    )

    # Create static library for runtime
    add_library(sn_runtime STATIC ${SN_RUNTIME_LIB_SOURCES})

    target_include_directories(sn_runtime PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/runtime
        ${CMAKE_SOURCE_DIR}/src/platform
    )

    # Platform-specific compile definitions
    if(NOT WIN32)
        target_compile_definitions(sn_runtime PRIVATE _GNU_SOURCE)
    else()
        # Force Windows API threading instead of pthreads for packaged builds
        target_compile_definitions(sn_runtime PRIVATE SN_USE_WIN32_THREADS)
    endif()

    # Optimization flags
    target_compile_options(sn_runtime PRIVATE -O2)

    # Output to lib directory with correct name (libsn_runtime.a on Unix, sn_runtime.lib on Windows)
    set_target_properties(sn_runtime PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${SN_RUNTIME_LIB_DIR}
        OUTPUT_NAME sn_runtime
    )
endif()

#------------------------------------------------------------------------------
# Setup Include and Config Directories
#------------------------------------------------------------------------------
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/bin/include)
set(RUNTIME_INCLUDE_DIR ${INCLUDE_DIR}/runtime)
set(PLATFORM_INCLUDE_DIR ${INCLUDE_DIR}/platform)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
file(MAKE_DIRECTORY ${INCLUDE_DIR})
file(MAKE_DIRECTORY ${RUNTIME_INCLUDE_DIR})
file(MAKE_DIRECTORY ${PLATFORM_INCLUDE_DIR})

# Copy unified config file (sn.cfg)
set(SN_CONFIG_FILE "${CMAKE_SOURCE_DIR}/etc/sn.cfg")
set(DEST_CONFIG "${CMAKE_SOURCE_DIR}/bin/sn.cfg")
if(EXISTS ${SN_CONFIG_FILE})
    if(NOT EXISTS ${DEST_CONFIG})
        file(INSTALL ${SN_CONFIG_FILE} DESTINATION "${CMAKE_SOURCE_DIR}/bin")
    endif()
endif()

# Copy runtime headers
if(EXISTS "${CMAKE_SOURCE_DIR}/src/runtime.h")
    file(INSTALL "${CMAKE_SOURCE_DIR}/src/runtime.h" DESTINATION "${INCLUDE_DIR}")
endif()

file(GLOB RUNTIME_HEADERS "${CMAKE_SOURCE_DIR}/src/runtime/*.h")
if(RUNTIME_HEADERS)
    file(INSTALL ${RUNTIME_HEADERS} DESTINATION "${RUNTIME_INCLUDE_DIR}")
endif()

# Copy platform headers
file(GLOB PLATFORM_HEADERS "${CMAKE_SOURCE_DIR}/src/platform/*.h")
if(PLATFORM_HEADERS)
    file(INSTALL ${PLATFORM_HEADERS} DESTINATION "${PLATFORM_INCLUDE_DIR}")
endif()

#------------------------------------------------------------------------------
# Setup SDK Directory
#------------------------------------------------------------------------------
set(SDK_DIR ${CMAKE_SOURCE_DIR}/bin/sdk)
file(MAKE_DIRECTORY ${SDK_DIR})

# Copy SDK files (.sn and .sn.c files)
file(GLOB SDK_SN_FILES "${CMAKE_SOURCE_DIR}/sdk/*.sn")
file(GLOB SDK_SN_C_FILES "${CMAKE_SOURCE_DIR}/sdk/*.sn.c")
if(SDK_SN_FILES)
    file(INSTALL ${SDK_SN_FILES} DESTINATION "${SDK_DIR}")
endif()
if(SDK_SN_C_FILES)
    file(INSTALL ${SDK_SN_C_FILES} DESTINATION "${SDK_DIR}")
endif()

#------------------------------------------------------------------------------
# Custom Targets
#------------------------------------------------------------------------------
# Run unit tests
add_custom_target(run-unit-tests
    COMMAND $<TARGET_FILE:tests>
    DEPENDS tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running unit tests"
)

# Run all tests using Python test runner
add_custom_target(run-tests
    COMMAND ${CMAKE_COMMAND} -E env PYTHONUNBUFFERED=1
        python3 ${CMAKE_SOURCE_DIR}/scripts/run_tests.py all
    DEPENDS sn tests sn_runtime
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running all tests"
)

# Run integration tests only
add_custom_target(run-integration-tests
    COMMAND ${CMAKE_COMMAND} -E env PYTHONUNBUFFERED=1
        python3 ${CMAKE_SOURCE_DIR}/scripts/run_tests.py integration
    DEPENDS sn sn_runtime
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running integration tests"
)

#------------------------------------------------------------------------------
# Installation Rules
#------------------------------------------------------------------------------
include(GNUInstallDirs)

# Install compiler
install(TARGETS sn
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT compiler
)

# Install runtime library
install(DIRECTORY "${CMAKE_SOURCE_DIR}/bin/lib/"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/sindarin
    COMPONENT runtime
    FILES_MATCHING PATTERN "*.a"
)

# Install headers
install(DIRECTORY "${CMAKE_SOURCE_DIR}/bin/include/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sindarin
    COMPONENT headers
)

# Install config file
install(FILES "${CMAKE_SOURCE_DIR}/etc/sn.cfg"
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/sindarin
    COMPONENT configs
)

# Install SDK files
install(DIRECTORY "${CMAKE_SOURCE_DIR}/sdk/"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/sindarin/sdk
    COMPONENT sdk
    FILES_MATCHING
    PATTERN "*.sn"
    PATTERN "*.sn.c"
)

#------------------------------------------------------------------------------
# Bundled Dependencies (zlib)
#------------------------------------------------------------------------------
# Option to bundle zlib with the package (for self-contained distribution)
option(SN_BUNDLE_ZLIB "Bundle zlib with the package" OFF)

if(SN_BUNDLE_ZLIB)
    # Look for zlib in common locations
    find_package(ZLIB QUIET)

    # Create deps directory for bundled files
    set(SN_DEPS_DIR ${CMAKE_SOURCE_DIR}/bin/deps)
    file(MAKE_DIRECTORY ${SN_DEPS_DIR})
    file(MAKE_DIRECTORY ${SN_DEPS_DIR}/include)
    file(MAKE_DIRECTORY ${SN_DEPS_DIR}/lib)

    if(ZLIB_FOUND)
        message(STATUS "Found zlib: ${ZLIB_LIBRARIES}")

        # Copy zlib headers
        if(ZLIB_INCLUDE_DIRS)
            file(GLOB ZLIB_HEADERS "${ZLIB_INCLUDE_DIRS}/zlib.h" "${ZLIB_INCLUDE_DIRS}/zconf.h")
            foreach(HEADER ${ZLIB_HEADERS})
                file(INSTALL ${HEADER} DESTINATION ${SN_DEPS_DIR}/include)
            endforeach()
        endif()

        # Copy zlib library (platform-specific)
        if(WIN32)
            # Windows: look for zlib1.dll
            get_filename_component(ZLIB_LIB_DIR ${ZLIB_LIBRARIES} DIRECTORY)
            file(GLOB ZLIB_DLLS "${ZLIB_LIB_DIR}/../bin/zlib*.dll" "${ZLIB_LIB_DIR}/zlib*.dll")
            foreach(DLL ${ZLIB_DLLS})
                file(INSTALL ${DLL} DESTINATION ${SN_DEPS_DIR}/lib)
            endforeach()
            # Also copy import library
            file(GLOB ZLIB_LIBS "${ZLIB_LIB_DIR}/zlib*.lib" "${ZLIB_LIB_DIR}/libz*.a")
            foreach(LIB ${ZLIB_LIBS})
                file(INSTALL ${LIB} DESTINATION ${SN_DEPS_DIR}/lib)
            endforeach()
        elseif(APPLE)
            # macOS: zlib is pre-installed, just note the path
            message(STATUS "macOS: Using system zlib (pre-installed)")
        else()
            # Linux: copy shared library
            get_filename_component(ZLIB_LIB_NAME ${ZLIB_LIBRARIES} NAME)
            file(INSTALL ${ZLIB_LIBRARIES} DESTINATION ${SN_DEPS_DIR}/lib)
            # Copy symlinks too
            get_filename_component(ZLIB_LIB_DIR ${ZLIB_LIBRARIES} DIRECTORY)
            file(GLOB ZLIB_SYMLINKS "${ZLIB_LIB_DIR}/libz.so*")
            foreach(LINK ${ZLIB_SYMLINKS})
                file(INSTALL ${LINK} DESTINATION ${SN_DEPS_DIR}/lib)
            endforeach()
        endif()
    else()
        message(STATUS "zlib not found - skipping bundling (SDK compression features may require manual zlib installation)")
    endif()

    # Install bundled deps
    if(EXISTS ${SN_DEPS_DIR})
        install(DIRECTORY ${SN_DEPS_DIR}/
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/sindarin/deps
            COMPONENT deps
        )
        set(CPACK_COMPONENTS_ALL compiler runtime headers configs sdk deps)
        set(CPACK_COMPONENT_DEPS_DISPLAY_NAME "Bundled Dependencies (zlib)")
        set(CPACK_COMPONENT_DEPS_DEPENDS runtime)
    endif()
endif()

#------------------------------------------------------------------------------
# CPack Configuration
#------------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "sindarin")
set(CPACK_PACKAGE_VENDOR "Sindarin Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Sindarin - A statically-typed language that compiles to C")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})

# Resource files
if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/README.md")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
endif()

# Components
set(CPACK_COMPONENTS_ALL compiler runtime headers configs sdk)
set(CPACK_COMPONENT_COMPILER_DISPLAY_NAME "Sindarin Compiler")
set(CPACK_COMPONENT_COMPILER_REQUIRED ON)
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Runtime Libraries")
set(CPACK_COMPONENT_RUNTIME_DEPENDS compiler)
set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "Development Headers")
set(CPACK_COMPONENT_HEADERS_DEPENDS runtime)
set(CPACK_COMPONENT_CONFIGS_DISPLAY_NAME "Configuration Files")
set(CPACK_COMPONENT_SDK_DISPLAY_NAME "SDK Standard Library")
set(CPACK_COMPONENT_SDK_DEPENDS compiler)

# Platform-specific generators
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-windows-x64")
elseif(APPLE)
    set(CPACK_GENERATOR "TGZ")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-macos-x64")
else()
    # Check if rpmbuild is available for RPM generation
    find_program(RPMBUILD_EXECUTABLE rpmbuild)
    if(RPMBUILD_EXECUTABLE)
        set(CPACK_GENERATOR "TGZ;DEB;RPM")
    else()
        set(CPACK_GENERATOR "TGZ;DEB")
        message(STATUS "rpmbuild not found - RPM package generation disabled")
    endif()
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-linux-x64")

    # DEB settings
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Sindarin Maintainers")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.17), gcc, libc6-dev, zlib1g-dev")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

    # RPM settings (only used if rpmbuild is available)
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Development/Languages")
    set(CPACK_RPM_PACKAGE_REQUIRES "gcc, glibc-devel, zlib-devel")
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
endif()

# Source package
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
    "/[.]git/"
    "/build/"
    "/bin/"
    "/log/"
    "/vcpkg/"
)

include(CPack)

#------------------------------------------------------------------------------
# Configuration Summary
#------------------------------------------------------------------------------
message(STATUS "")
message(STATUS "Sindarin Compiler Build Configuration:")
message(STATUS "  Version:      ${PROJECT_VERSION}")
message(STATUS "  Platform:     ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler:     ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  Debug:        ${SN_DEBUG}")
message(STATUS "  ASAN:         ${SN_ASAN}")
message(STATUS "  Output:       ${CMAKE_SOURCE_DIR}/bin")
message(STATUS "")
