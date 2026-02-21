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

# Generate version header from template
configure_file(
    ${CMAKE_SOURCE_DIR}/src/version.h.in
    ${CMAKE_BINARY_DIR}/generated/version.h
    @ONLY
)
include_directories(${CMAKE_BINARY_DIR}/generated)

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
    add_compile_options(-O3 -flto=auto)
    add_link_options(-flto=auto -O3)
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
    # util/
    src/parser/util/parser_util.c
    # expr/
    src/parser/expr/parser_expr.c
    src/parser/expr/parser_expr_interpol.c
    src/parser/expr/parser_expr_match.c
    src/parser/expr/parser_expr_struct.c
    src/parser/expr/parser_expr_lambda.c
    # stmt/
    src/parser/stmt/parser_stmt.c
    src/parser/stmt/parser_stmt_decl.c
    src/parser/stmt/parser_stmt_control.c
)

set(SN_SYMBOL_TABLE_SOURCES
    src/symbol_table.c
    src/symbol_table/symbol_table_core.c
    src/symbol_table/symbol_table_namespace.c
    src/symbol_table/symbol_table_thread.c
)

set(SN_TYPE_CHECKER_SOURCES
    src/type_checker.c
    # util/
    src/type_checker/util/type_checker_util.c
    src/type_checker/util/type_checker_util_escape.c
    src/type_checker/util/type_checker_util_suggest.c
    src/type_checker/util/type_checker_util_struct.c
    src/type_checker/util/type_checker_util_layout.c
    # expr/
    src/type_checker/expr/type_checker_expr.c
    src/type_checker/expr/type_checker_expr_ops.c
    src/type_checker/expr/type_checker_expr_assign.c
    src/type_checker/expr/type_checker_expr_member.c
    src/type_checker/expr/type_checker_expr_thread.c
    src/type_checker/expr/type_checker_expr_basic.c
    src/type_checker/expr/type_checker_expr_cast.c
    src/type_checker/expr/type_checker_expr_struct.c
    src/type_checker/expr/type_checker_expr_access.c
    src/type_checker/expr/type_checker_expr_misc.c
    src/type_checker/expr/type_checker_expr_array.c
    src/type_checker/expr/type_checker_expr_lambda.c
    # expr/call/
    src/type_checker/expr/call/type_checker_expr_call.c
    src/type_checker/expr/call/type_checker_expr_call_core.c
    src/type_checker/expr/call/type_checker_expr_call_array.c
    src/type_checker/expr/call/type_checker_expr_call_char.c
    src/type_checker/expr/call/type_checker_expr_call_string.c
    # stmt/
    src/type_checker/stmt/type_checker_stmt.c
    src/type_checker/stmt/type_checker_stmt_var.c
    src/type_checker/stmt/type_checker_stmt_var_util.c
    src/type_checker/stmt/type_checker_stmt_func.c
    src/type_checker/stmt/type_checker_stmt_control.c
    src/type_checker/stmt/type_checker_stmt_struct.c
    src/type_checker/stmt/type_checker_stmt_import.c
    src/type_checker/stmt/type_checker_stmt_import_ns.c
    src/type_checker/stmt/type_checker_stmt_import_ns_util.c
)

set(SN_OPTIMIZER_SOURCES
    src/optimizer.c
    src/optimizer/optimizer_util.c
    src/optimizer/optimizer_tail_call.c
    src/optimizer/optimizer_string.c
)

set(SN_CODEGEN_SOURCES
    src/code_gen.c
    # emit/
    src/code_gen/emit/code_gen_pragma.c
    src/code_gen/emit/code_gen_struct_emit.c
    src/code_gen/emit/code_gen_native_extern.c
    src/code_gen/emit/code_gen_method_emit.c
    # util/
    src/code_gen/util/code_gen_util_string.c
    src/code_gen/util/code_gen_util_type.c
    src/code_gen/util/code_gen_util_tostring.c
    src/code_gen/util/code_gen_util_boxing.c
    src/code_gen/util/code_gen_util_fold.c
    src/code_gen/util/code_gen_util_native.c
    src/code_gen/util/code_gen_util_arena.c
    src/code_gen/util/code_gen_util_promote.c
    src/code_gen/util/code_gen_util_callbacks.c
    src/code_gen/util/code_gen_util_temps.c
    # expr/
    src/code_gen/expr/code_gen_expr.c
    src/code_gen/expr/code_gen_expr_core.c
    src/code_gen/expr/code_gen_expr_binary.c
    src/code_gen/expr/code_gen_expr_array.c
    src/code_gen/expr/code_gen_expr_static.c
    src/code_gen/expr/code_gen_expr_string.c
    src/code_gen/expr/code_gen_expr_incr.c
    src/code_gen/expr/code_gen_expr_member.c
    src/code_gen/expr/code_gen_expr_misc.c
    src/code_gen/expr/code_gen_expr_type.c
    src/code_gen/expr/code_gen_expr_struct.c
    src/code_gen/expr/code_gen_expr_access.c
    src/code_gen/expr/code_gen_expr_match.c
    # expr/call/
    src/code_gen/expr/call/code_gen_expr_call.c
    src/code_gen/expr/call/code_gen_expr_call_namespace.c
    src/code_gen/expr/call/code_gen_expr_call_struct.c
    src/code_gen/expr/call/code_gen_expr_call_closure.c
    src/code_gen/expr/call/code_gen_expr_call_builtin.c
    src/code_gen/expr/call/code_gen_expr_call_array.c
    # Note: code_gen_expr_call_array_{byte,mutate,push,query}.c are #included by code_gen_expr_call_array.c
    src/code_gen/expr/call/code_gen_expr_call_string.c
    src/code_gen/expr/call/code_gen_expr_call_intercept.c
    # Note: code_gen_expr_call_intercept_{func,method}.c are #included by code_gen_expr_call_intercept.c
    src/code_gen/expr/call/code_gen_expr_call_char.c
    src/code_gen/expr/call/code_gen_expr_call_byte_array.c
    # expr/lambda/
    src/code_gen/expr/lambda/code_gen_expr_lambda.c
    # Note: code_gen_expr_lambda_{capture,local,native}.c are #included by code_gen_expr_lambda.c
    # expr/thread/
    src/code_gen/expr/thread/code_gen_expr_thread.c
    src/code_gen/expr/thread/code_gen_expr_thread_util.c
    src/code_gen/expr/thread/code_gen_expr_thread_sync.c
    # stmt/
    src/code_gen/stmt/code_gen_stmt_core.c
    src/code_gen/stmt/code_gen_stmt_loop.c
    src/code_gen/stmt/code_gen_stmt_capture.c
    src/code_gen/stmt/code_gen_stmt_import.c
    src/code_gen/stmt/code_gen_stmt_thread.c
    src/code_gen/stmt/code_gen_stmt_struct.c
    src/code_gen/stmt/code_gen_stmt_var_init.c
    src/code_gen/stmt/code_gen_stmt_var.c
    src/code_gen/stmt/code_gen_stmt_func_promote.c
    src/code_gen/stmt/code_gen_stmt_func.c
    src/code_gen/stmt/code_gen_stmt_return.c
)

set(SN_BACKEND_SOURCES
    src/gcc_backend.c
    src/gcc_backend_config.c
    src/gcc_backend_pkgconfig.c
)

set(SN_PACKAGE_SOURCES
    src/package.c
    src/package/package_yaml.c
    src/package/package_git.c
    src/package/lfs/package_lfs.c
)

set(SN_RUNTIME_SOURCES
    src/runtime.c
    # arena/
    src/runtime/arena/arena_v2.c
    src/runtime/arena/arena_id.c
    src/runtime/arena/arena_stats.c
    src/runtime/arena/arena_gc.c
    src/runtime/arena/arena_handle.c
    src/runtime/arena/arena_redirect.c
    # string/
    src/runtime/string/runtime_string.c
    src/runtime/string/runtime_string_v2.c
    # array/
    src/runtime/array/runtime_array_v2_core.c
    src/runtime/array/runtime_array_v2_string.c
    src/runtime/array/runtime_array_v2_alloc.c
    src/runtime/array/runtime_array_v2_join.c
    src/runtime/array/runtime_array_v2_tostring.c
    src/runtime/array/runtime_array_v2_any.c
    src/runtime/array/runtime_array_v2_print.c
    # thread/
    src/runtime/thread/runtime_thread_v3.c
    # core
    src/runtime/runtime_io.c
    src/runtime/runtime_byte.c
    src/runtime/runtime_any.c
    src/runtime/runtime_intercept.c
    # malloc/ (needed by arena_redirect.c)
    src/runtime/malloc/runtime_malloc_hooks.c
)

# Platform-specific hooking libraries for SN_RUNTIME_SOURCES
if(APPLE)
    list(APPEND SN_RUNTIME_SOURCES src/runtime/hooks/fishhook.c)
elseif(WIN32)
    list(APPEND SN_RUNTIME_SOURCES
        src/runtime/hooks/minhook/hook.c
        src/runtime/hooks/minhook/buffer.c
        src/runtime/hooks/minhook/trampoline.c
        src/runtime/hooks/minhook/hde/hde32.c
        src/runtime/hooks/minhook/hde/hde64.c
    )
else()
    list(APPEND SN_RUNTIME_SOURCES src/runtime/hooks/plthook_elf.c)
endif()

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
    ${SN_PACKAGE_SOURCES}
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
    ${CMAKE_SOURCE_DIR}/src/runtime/hooks
    ${CMAKE_SOURCE_DIR}/src/platform
)

# Windows: Add MinHook include paths
if(WIN32)
    target_include_directories(sn PRIVATE
        ${CMAKE_SOURCE_DIR}/src/runtime/hooks/minhook
        ${CMAKE_SOURCE_DIR}/src/runtime/hooks/minhook/hde
    )
endif()


# Platform-specific libraries
if(WIN32)
    target_link_libraries(sn PRIVATE ws2_32 bcrypt pthread)
else()
    target_link_libraries(sn PRIVATE pthread m)
endif()

#------------------------------------------------------------------------------
# libcurl for Auto-Update Feature (detected after libs submodule section below)
#------------------------------------------------------------------------------
# Curl detection is deferred until after libs submodule is detected

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
    ${CMAKE_SOURCE_DIR}/src/runtime/hooks
    ${CMAKE_SOURCE_DIR}/src/platform
)

# Windows: Add MinHook include paths
if(WIN32)
    target_include_directories(tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src/runtime/hooks/minhook
        ${CMAKE_SOURCE_DIR}/src/runtime/hooks/minhook/hde
    )
endif()

# Suppress unused-variable warnings in test code - test patterns intentionally
# allocate variables to exercise allocation paths without reading them back.
target_compile_options(tests PRIVATE -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function)

if(WIN32)
    target_link_libraries(tests PRIVATE ws2_32 bcrypt pthread)
else()
    target_link_libraries(tests PRIVATE pthread m)
endif()

# Note: Additional library linking for tests will be done after libs submodule detection below

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
        # arena/
        ${CMAKE_SOURCE_DIR}/src/runtime/arena/arena_v2.c
        ${CMAKE_SOURCE_DIR}/src/runtime/arena/arena_id.c
        ${CMAKE_SOURCE_DIR}/src/runtime/arena/arena_stats.c
        ${CMAKE_SOURCE_DIR}/src/runtime/arena/arena_gc.c
        ${CMAKE_SOURCE_DIR}/src/runtime/arena/arena_handle.c
        ${CMAKE_SOURCE_DIR}/src/runtime/arena/arena_redirect.c
        # string/
        ${CMAKE_SOURCE_DIR}/src/runtime/string/runtime_string.c
        ${CMAKE_SOURCE_DIR}/src/runtime/string/runtime_string_v2.c
        # array/
        ${CMAKE_SOURCE_DIR}/src/runtime/array/runtime_array_v2_core.c
        ${CMAKE_SOURCE_DIR}/src/runtime/array/runtime_array_v2_string.c
        ${CMAKE_SOURCE_DIR}/src/runtime/array/runtime_array_v2_alloc.c
        ${CMAKE_SOURCE_DIR}/src/runtime/array/runtime_array_v2_join.c
        ${CMAKE_SOURCE_DIR}/src/runtime/array/runtime_array_v2_tostring.c
        ${CMAKE_SOURCE_DIR}/src/runtime/array/runtime_array_v2_any.c
        ${CMAKE_SOURCE_DIR}/src/runtime/array/runtime_array_v2_print.c
        # thread/
        ${CMAKE_SOURCE_DIR}/src/runtime/thread/runtime_thread_v3.c
        # core
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_io.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_byte.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_any.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_intercept.c
        # malloc/
        ${CMAKE_SOURCE_DIR}/src/runtime/malloc/runtime_malloc_hooks.c
        ${CMAKE_SOURCE_DIR}/src/runtime/malloc/runtime_malloc_redirect.c
    )

    # Platform-specific hooking libraries for malloc interception
    if(APPLE)
        # macOS: fishhook for runtime symbol rebinding
        list(APPEND SN_RUNTIME_LIB_SOURCES
            ${CMAKE_SOURCE_DIR}/src/runtime/hooks/fishhook.c
        )
    elseif(WIN32)
        # Windows: MinHook for inline function hooking
        list(APPEND SN_RUNTIME_LIB_SOURCES
            ${CMAKE_SOURCE_DIR}/src/runtime/hooks/minhook/hook.c
            ${CMAKE_SOURCE_DIR}/src/runtime/hooks/minhook/buffer.c
            ${CMAKE_SOURCE_DIR}/src/runtime/hooks/minhook/trampoline.c
            ${CMAKE_SOURCE_DIR}/src/runtime/hooks/minhook/hde/hde32.c
            ${CMAKE_SOURCE_DIR}/src/runtime/hooks/minhook/hde/hde64.c
        )
    else()
        # Linux: plthook for PLT/GOT modification
        list(APPEND SN_RUNTIME_LIB_SOURCES
            ${CMAKE_SOURCE_DIR}/src/runtime/hooks/plthook_elf.c
        )
    endif()

    # Create static library for runtime
    add_library(sn_runtime STATIC ${SN_RUNTIME_LIB_SOURCES})

    target_include_directories(sn_runtime PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/runtime
        ${CMAKE_SOURCE_DIR}/src/runtime/hooks
        ${CMAKE_SOURCE_DIR}/src/platform
    )

    # Windows: Add MinHook include paths
    if(WIN32)
        target_include_directories(sn_runtime PRIVATE
            ${CMAKE_SOURCE_DIR}/src/runtime/hooks/minhook
            ${CMAKE_SOURCE_DIR}/src/runtime/hooks/minhook/hde
        )
    endif()

    # Platform-specific compile definitions
    if(NOT WIN32)
        target_compile_definitions(sn_runtime PRIVATE _GNU_SOURCE)
    else()
        # Force Windows API threading instead of pthreads for packaged builds
        target_compile_definitions(sn_runtime PRIVATE SN_USE_WIN32_THREADS)
    endif()


    # Optimization flags — use -fno-lto so runtime works with both -flto and non-flto user builds
    target_compile_options(sn_runtime PRIVATE -O2 -fno-lto)

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

# Copy arena headers (managed arena)
set(ARENA_INCLUDE_DIR ${RUNTIME_INCLUDE_DIR}/arena)
file(MAKE_DIRECTORY ${ARENA_INCLUDE_DIR})
file(GLOB ARENA_HEADERS "${CMAKE_SOURCE_DIR}/src/runtime/arena/*.h")
if(ARENA_HEADERS)
    file(INSTALL ${ARENA_HEADERS} DESTINATION "${ARENA_INCLUDE_DIR}")
endif()

# Copy string headers
set(STRING_INCLUDE_DIR ${RUNTIME_INCLUDE_DIR}/string)
file(MAKE_DIRECTORY ${STRING_INCLUDE_DIR})
file(GLOB STRING_HEADERS "${CMAKE_SOURCE_DIR}/src/runtime/string/*.h")
if(STRING_HEADERS)
    file(INSTALL ${STRING_HEADERS} DESTINATION "${STRING_INCLUDE_DIR}")
endif()

# Copy array headers
set(ARRAY_INCLUDE_DIR ${RUNTIME_INCLUDE_DIR}/array)
file(MAKE_DIRECTORY ${ARRAY_INCLUDE_DIR})
file(GLOB ARRAY_HEADERS "${CMAKE_SOURCE_DIR}/src/runtime/array/*.h")
if(ARRAY_HEADERS)
    file(INSTALL ${ARRAY_HEADERS} DESTINATION "${ARRAY_INCLUDE_DIR}")
endif()

# Copy thread headers
set(THREAD_INCLUDE_DIR ${RUNTIME_INCLUDE_DIR}/thread)
file(MAKE_DIRECTORY ${THREAD_INCLUDE_DIR})
file(GLOB THREAD_HEADERS "${CMAKE_SOURCE_DIR}/src/runtime/thread/*.h")
if(THREAD_HEADERS)
    file(INSTALL ${THREAD_HEADERS} DESTINATION "${THREAD_INCLUDE_DIR}")
endif()

# Copy malloc headers
set(MALLOC_INCLUDE_DIR ${RUNTIME_INCLUDE_DIR}/malloc)
file(MAKE_DIRECTORY ${MALLOC_INCLUDE_DIR})
file(GLOB MALLOC_HEADERS "${CMAKE_SOURCE_DIR}/src/runtime/malloc/*.h")
if(MALLOC_HEADERS)
    file(INSTALL ${MALLOC_HEADERS} DESTINATION "${MALLOC_INCLUDE_DIR}")
endif()

# Arena V2 headers are now in src/runtime/arena/ and copied above

# Copy platform headers
file(GLOB PLATFORM_HEADERS "${CMAKE_SOURCE_DIR}/src/platform/*.h")
if(PLATFORM_HEADERS)
    file(INSTALL ${PLATFORM_HEADERS} DESTINATION "${PLATFORM_INCLUDE_DIR}")
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
# All Sindarin resources are installed to a single SDK root directory:
#   <prefix>/lib/sindarin/
# The compiler binary is installed there too, with a symlink from bin/.
# This allows the compiler to find all resources relative to its own real path.
include(GNUInstallDirs)

set(SN_SDK_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/sindarin)

# Install compiler binary into SDK root
install(TARGETS sn
    RUNTIME DESTINATION ${SN_SDK_INSTALL_DIR}
    COMPONENT compiler
)

# Create bin/sn entry point
# On Windows: copy the executable (symlinks don't work reliably)
# On Unix: create symlink to preserve the SDK-relative path resolution
if(WIN32)
    install(CODE "
        set(EXE_SOURCE \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${SN_SDK_INSTALL_DIR}/sn.exe\")
        set(EXE_DEST \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/sn.exe\")
        file(MAKE_DIRECTORY \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\")
        execute_process(COMMAND \${CMAKE_COMMAND} -E copy \${EXE_SOURCE} \${EXE_DEST})
    " COMPONENT compiler)
else()
    install(CODE "
        set(LINK_SOURCE \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/sn\")
        set(LINK_TARGET \"../${SN_SDK_INSTALL_DIR}/sn\")
        file(MAKE_DIRECTORY \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\")
        execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink \${LINK_TARGET} \${LINK_SOURCE})
    " COMPONENT compiler)
endif()

# Install runtime libraries
install(DIRECTORY "${CMAKE_SOURCE_DIR}/bin/lib/"
    DESTINATION ${SN_SDK_INSTALL_DIR}/lib
    COMPONENT runtime
    FILES_MATCHING PATTERN "*.a"
)

# Install headers
install(DIRECTORY "${CMAKE_SOURCE_DIR}/bin/include/"
    DESTINATION ${SN_SDK_INSTALL_DIR}/include
    COMPONENT headers
)

# Install config file
install(FILES "${CMAKE_SOURCE_DIR}/etc/sn.cfg"
    DESTINATION ${SN_SDK_INSTALL_DIR}
    COMPONENT configs
)

#------------------------------------------------------------------------------
# Dependencies Detection (sindarin-libs submodule)
#------------------------------------------------------------------------------
# Pre-built libraries are provided via the libs submodule
# Run 'make setup' to initialize the submodule if not present

# Determine platform for sindarin-libs
if(WIN32)
    set(SINDARIN_LIBS_PLATFORM "windows")
elseif(APPLE)
    set(SINDARIN_LIBS_PLATFORM "darwin")
else()
    set(SINDARIN_LIBS_PLATFORM "linux")
endif()

# Find dependency installation directory from libs
set(SINDARIN_LIBS_DIR "${CMAKE_SOURCE_DIR}/libs/libs/${SINDARIN_LIBS_PLATFORM}")

# Create bin/deps directory structure
set(SN_DEPS_DIR ${CMAKE_SOURCE_DIR}/bin/deps)
file(MAKE_DIRECTORY ${SN_DEPS_DIR})
file(MAKE_DIRECTORY ${SN_DEPS_DIR}/include)
file(MAKE_DIRECTORY ${SN_DEPS_DIR}/lib)

# Copy dependencies if found
if(EXISTS "${SINDARIN_LIBS_DIR}/lib" AND EXISTS "${SINDARIN_LIBS_DIR}/include")
    message(STATUS "Found libs submodule: ${SINDARIN_LIBS_DIR}")

    # Copy headers
    if(EXISTS "${SINDARIN_LIBS_DIR}/include/zlib.h")
        message(STATUS "  Copying zlib headers...")
        file(INSTALL "${SINDARIN_LIBS_DIR}/include/zlib.h" DESTINATION ${SN_DEPS_DIR}/include)
        file(INSTALL "${SINDARIN_LIBS_DIR}/include/zconf.h" DESTINATION ${SN_DEPS_DIR}/include)
    endif()

    if(EXISTS "${SINDARIN_LIBS_DIR}/include/yyjson.h")
        message(STATUS "  Copying yyjson headers...")
        file(INSTALL "${SINDARIN_LIBS_DIR}/include/yyjson.h" DESTINATION ${SN_DEPS_DIR}/include)
    endif()

    if(EXISTS "${SINDARIN_LIBS_DIR}/include/libxml2/libxml")
        message(STATUS "  Copying libxml2 headers...")
        file(INSTALL "${SINDARIN_LIBS_DIR}/include/libxml2/libxml" DESTINATION ${SN_DEPS_DIR}/include)
    endif()

    if(EXISTS "${SINDARIN_LIBS_DIR}/include/yaml.h")
        message(STATUS "  Copying libyaml headers...")
        file(INSTALL "${SINDARIN_LIBS_DIR}/include/yaml.h" DESTINATION ${SN_DEPS_DIR}/include)
    endif()

    if(EXISTS "${SINDARIN_LIBS_DIR}/include/openssl/ssl.h")
        message(STATUS "  Copying openssl headers...")
        file(INSTALL "${SINDARIN_LIBS_DIR}/include/openssl" DESTINATION ${SN_DEPS_DIR}/include)
    endif()

    if(EXISTS "${SINDARIN_LIBS_DIR}/include/ngtcp2/ngtcp2.h")
        message(STATUS "  Copying ngtcp2 headers...")
        file(INSTALL "${SINDARIN_LIBS_DIR}/include/ngtcp2" DESTINATION ${SN_DEPS_DIR}/include)
    endif()

    if(EXISTS "${SINDARIN_LIBS_DIR}/include/libssh/libssh.h")
        message(STATUS "  Copying libssh headers...")
        file(INSTALL "${SINDARIN_LIBS_DIR}/include/libssh" DESTINATION ${SN_DEPS_DIR}/include)
    endif()

    if(EXISTS "${SINDARIN_LIBS_DIR}/include/git2.h")
        message(STATUS "  Copying libgit2 headers...")
        file(INSTALL "${SINDARIN_LIBS_DIR}/include/git2.h" DESTINATION ${SN_DEPS_DIR}/include)
        file(INSTALL "${SINDARIN_LIBS_DIR}/include/git2" DESTINATION ${SN_DEPS_DIR}/include)
    endif()

    if(EXISTS "${SINDARIN_LIBS_DIR}/include/curl/curl.h")
        message(STATUS "  Copying curl headers...")
        file(INSTALL "${SINDARIN_LIBS_DIR}/include/curl" DESTINATION ${SN_DEPS_DIR}/include)
    endif()

    # Copy libraries based on platform
    if(WIN32)
        # Windows (MinGW): Copy .a static libraries and .dll if present
        file(GLOB ZLIB_LIBS "${SINDARIN_LIBS_DIR}/lib/libz*.a" "${SINDARIN_LIBS_DIR}/lib/zlib*.a")
        file(GLOB YYJSON_LIBS "${SINDARIN_LIBS_DIR}/lib/libyyjson*.a" "${SINDARIN_LIBS_DIR}/lib/yyjson*.a")
        file(GLOB XML2_LIBS "${SINDARIN_LIBS_DIR}/lib/libxml2*.a" "${SINDARIN_LIBS_DIR}/lib/xml2*.a")
        file(GLOB YAML_LIBS "${SINDARIN_LIBS_DIR}/lib/libyaml*.a" "${SINDARIN_LIBS_DIR}/lib/yaml*.a")
        file(GLOB OPENSSL_LIBS "${SINDARIN_LIBS_DIR}/lib/libssl*.a" "${SINDARIN_LIBS_DIR}/lib/libcrypto*.a")
        file(GLOB NGTCP2_LIBS "${SINDARIN_LIBS_DIR}/lib/libngtcp2*.a")
        file(GLOB SSH_LIBS "${SINDARIN_LIBS_DIR}/lib/libssh*.a")
        file(GLOB GIT2_LIBS "${SINDARIN_LIBS_DIR}/lib/libgit2*.a" "${SINDARIN_LIBS_DIR}/lib/git2*.a")
        file(GLOB GIT2_DEPS_LIBS "${SINDARIN_LIBS_DIR}/lib/libhttp_parser*.a" "${SINDARIN_LIBS_DIR}/lib/libpcre2-8*.a" "${SINDARIN_LIBS_DIR}/lib/libssh2*.a")
        file(GLOB CURL_LIBS "${SINDARIN_LIBS_DIR}/lib/libcurl*.a" "${SINDARIN_LIBS_DIR}/lib/curl*.a")
        file(GLOB ZLIB_DLLS "${SINDARIN_LIBS_DIR}/bin/libz*.dll" "${SINDARIN_LIBS_DIR}/bin/zlib*.dll")
        file(GLOB YYJSON_DLLS "${SINDARIN_LIBS_DIR}/bin/libyyjson*.dll" "${SINDARIN_LIBS_DIR}/bin/yyjson*.dll")

        foreach(LIB ${ZLIB_LIBS} ${YYJSON_LIBS} ${XML2_LIBS} ${YAML_LIBS} ${OPENSSL_LIBS} ${NGTCP2_LIBS} ${SSH_LIBS} ${GIT2_LIBS} ${GIT2_DEPS_LIBS} ${CURL_LIBS})
            message(STATUS "  Copying library: ${LIB}")
            file(INSTALL ${LIB} DESTINATION ${SN_DEPS_DIR}/lib)
        endforeach()
        foreach(DLL ${ZLIB_DLLS} ${YYJSON_DLLS})
            message(STATUS "  Copying DLL: ${DLL}")
            file(INSTALL ${DLL} DESTINATION ${SN_DEPS_DIR}/lib)
        endforeach()
    elseif(APPLE)
        # macOS: Copy static or dynamic libraries
        file(GLOB ZLIB_LIBS "${SINDARIN_LIBS_DIR}/lib/libz.*")
        file(GLOB YYJSON_LIBS "${SINDARIN_LIBS_DIR}/lib/libyyjson.*")
        file(GLOB XML2_LIBS "${SINDARIN_LIBS_DIR}/lib/libxml2.*")
        file(GLOB YAML_LIBS "${SINDARIN_LIBS_DIR}/lib/libyaml.*")
        file(GLOB OPENSSL_LIBS "${SINDARIN_LIBS_DIR}/lib/libssl.*" "${SINDARIN_LIBS_DIR}/lib/libcrypto.*")
        file(GLOB NGTCP2_LIBS "${SINDARIN_LIBS_DIR}/lib/libngtcp2*.*")
        file(GLOB SSH_LIBS "${SINDARIN_LIBS_DIR}/lib/libssh.*")
        file(GLOB GIT2_LIBS "${SINDARIN_LIBS_DIR}/lib/libgit2.*")
        file(GLOB GIT2_DEPS_LIBS "${SINDARIN_LIBS_DIR}/lib/libhttp_parser.*" "${SINDARIN_LIBS_DIR}/lib/libpcre2-8.*" "${SINDARIN_LIBS_DIR}/lib/libssh2.*")
        file(GLOB CURL_LIBS "${SINDARIN_LIBS_DIR}/lib/libcurl.*")

        foreach(LIB ${ZLIB_LIBS} ${YYJSON_LIBS} ${XML2_LIBS} ${YAML_LIBS} ${OPENSSL_LIBS} ${NGTCP2_LIBS} ${SSH_LIBS} ${GIT2_LIBS} ${GIT2_DEPS_LIBS} ${CURL_LIBS})
            message(STATUS "  Copying library: ${LIB}")
            file(INSTALL ${LIB} DESTINATION ${SN_DEPS_DIR}/lib)
        endforeach()
    else()
        # Linux: Copy static libraries (.a files) for self-contained executables
        file(GLOB ZLIB_LIBS "${SINDARIN_LIBS_DIR}/lib/libz.a")
        file(GLOB YYJSON_LIBS "${SINDARIN_LIBS_DIR}/lib/libyyjson.a")
        file(GLOB XML2_LIBS "${SINDARIN_LIBS_DIR}/lib/libxml2.a")
        file(GLOB YAML_LIBS "${SINDARIN_LIBS_DIR}/lib/libyaml.a")
        file(GLOB OPENSSL_LIBS "${SINDARIN_LIBS_DIR}/lib/libssl.a" "${SINDARIN_LIBS_DIR}/lib/libcrypto.a")
        file(GLOB NGTCP2_LIBS "${SINDARIN_LIBS_DIR}/lib/libngtcp2*.a")
        file(GLOB SSH_LIBS "${SINDARIN_LIBS_DIR}/lib/libssh.a")
        file(GLOB GIT2_LIBS "${SINDARIN_LIBS_DIR}/lib/libgit2.a")
        file(GLOB GIT2_DEPS_LIBS "${SINDARIN_LIBS_DIR}/lib/libhttp_parser.a" "${SINDARIN_LIBS_DIR}/lib/libpcre2-8.a" "${SINDARIN_LIBS_DIR}/lib/libssh2.a")
        file(GLOB CURL_LIBS "${SINDARIN_LIBS_DIR}/lib/libcurl.a")

        foreach(LIB ${ZLIB_LIBS} ${YYJSON_LIBS} ${XML2_LIBS} ${YAML_LIBS} ${OPENSSL_LIBS} ${NGTCP2_LIBS} ${SSH_LIBS} ${GIT2_LIBS} ${GIT2_DEPS_LIBS} ${CURL_LIBS})
            message(STATUS "  Copying library: ${LIB}")
            file(INSTALL ${LIB} DESTINATION ${SN_DEPS_DIR}/lib)
        endforeach()
    endif()

    message(STATUS "  Dependencies copied to: ${SN_DEPS_DIR}")

    # Check for libcurl and enable auto-update feature
    if(EXISTS "${SINDARIN_LIBS_DIR}/include/curl/curl.h")
        message(STATUS "Found libcurl - enabling auto-update feature")
        target_include_directories(sn PRIVATE "${SINDARIN_LIBS_DIR}/include")
        target_compile_definitions(sn PRIVATE SN_HAS_CURL=1)

        # Link curl and its dependencies
        if(WIN32)
            # Windows: link curl with SSL and Windows socket libraries
            target_link_libraries(sn PRIVATE
                "${SINDARIN_LIBS_DIR}/lib/libcurl.a"
                "${SINDARIN_LIBS_DIR}/lib/libssl.a"
                "${SINDARIN_LIBS_DIR}/lib/libcrypto.a"
                "${SINDARIN_LIBS_DIR}/lib/libzlib.a"
                crypt32 wldap32 iphlpapi secur32
            )
        elseif(APPLE)
            # macOS: link curl with SSL and required system frameworks
            target_link_libraries(sn PRIVATE
                "${SINDARIN_LIBS_DIR}/lib/libcurl.a"
                "${SINDARIN_LIBS_DIR}/lib/libssl.a"
                "${SINDARIN_LIBS_DIR}/lib/libcrypto.a"
                "${SINDARIN_LIBS_DIR}/lib/libz.a"
                "-framework CoreFoundation"
                "-framework SystemConfiguration"
                "-framework Security"
            )
        else()
            # Linux: link curl with SSL
            target_link_libraries(sn PRIVATE
                "${SINDARIN_LIBS_DIR}/lib/libcurl.a"
                "${SINDARIN_LIBS_DIR}/lib/libssl.a"
                "${SINDARIN_LIBS_DIR}/lib/libcrypto.a"
                "${SINDARIN_LIBS_DIR}/lib/libz.a"
            )
        endif()

        # Link yyjson for JSON parsing
        if(WIN32)
            target_link_libraries(sn PRIVATE "${SINDARIN_LIBS_DIR}/lib/libyyjson.a")
        else()
            target_link_libraries(sn PRIVATE "${SINDARIN_LIBS_DIR}/lib/libyyjson.a")
        endif()
    else()
        message(STATUS "libcurl not found - auto-update feature disabled")
        target_compile_definitions(sn PRIVATE SN_HAS_CURL=0)
    endif()

    # Check for libgit2 and libyaml for package manager
    if(EXISTS "${SINDARIN_LIBS_DIR}/include/git2.h" AND EXISTS "${SINDARIN_LIBS_DIR}/include/yaml.h")
        message(STATUS "Found libgit2 and libyaml - enabling package manager")
        target_compile_definitions(sn PRIVATE SN_HAS_PACKAGE_MANAGER=1)

        # Link libgit2 and its dependencies
        if(WIN32)
            target_link_libraries(sn PRIVATE
                "${SINDARIN_LIBS_DIR}/lib/libgit2.a"
                "${SINDARIN_LIBS_DIR}/lib/libyaml.a"
                "${SINDARIN_LIBS_DIR}/lib/libssh2.a"
                "${SINDARIN_LIBS_DIR}/lib/libpcre2-8.a"
                "${SINDARIN_LIBS_DIR}/lib/libhttp_parser.a"
            )
        elseif(APPLE)
            target_link_libraries(sn PRIVATE
                "${SINDARIN_LIBS_DIR}/lib/libgit2.a"
                "${SINDARIN_LIBS_DIR}/lib/libyaml.a"
                "${SINDARIN_LIBS_DIR}/lib/libssh2.a"
                "${SINDARIN_LIBS_DIR}/lib/libpcre2-8.a"
                "${SINDARIN_LIBS_DIR}/lib/libhttp_parser.a"
                "-framework Security"
                "-framework CoreFoundation"
                iconv
            )
        else()
            target_link_libraries(sn PRIVATE
                "${SINDARIN_LIBS_DIR}/lib/libgit2.a"
                "${SINDARIN_LIBS_DIR}/lib/libyaml.a"
                "${SINDARIN_LIBS_DIR}/lib/libssh2.a"
                "${SINDARIN_LIBS_DIR}/lib/libpcre2-8.a"
                "${SINDARIN_LIBS_DIR}/lib/libhttp_parser.a"
            )
        endif()
    else()
        message(STATUS "libgit2 or libyaml not found - package manager disabled")
        target_compile_definitions(sn PRIVATE SN_HAS_PACKAGE_MANAGER=0)
    endif()

    # Link tests executable with libraries from libs submodule
    target_include_directories(tests PRIVATE "${SINDARIN_LIBS_DIR}/include")
    if(EXISTS "${SINDARIN_LIBS_DIR}/include/curl/curl.h")
        target_compile_definitions(tests PRIVATE SN_HAS_CURL=1)
        if(WIN32)
            target_link_libraries(tests PRIVATE
                "${SINDARIN_LIBS_DIR}/lib/libcurl.a"
                "${SINDARIN_LIBS_DIR}/lib/libssl.a"
                "${SINDARIN_LIBS_DIR}/lib/libcrypto.a"
                "${SINDARIN_LIBS_DIR}/lib/libzlib.a"
                "${SINDARIN_LIBS_DIR}/lib/libyyjson.a"
                crypt32 wldap32 iphlpapi secur32
            )
        elseif(APPLE)
            target_link_libraries(tests PRIVATE
                "${SINDARIN_LIBS_DIR}/lib/libcurl.a"
                "${SINDARIN_LIBS_DIR}/lib/libssl.a"
                "${SINDARIN_LIBS_DIR}/lib/libcrypto.a"
                "${SINDARIN_LIBS_DIR}/lib/libz.a"
                "${SINDARIN_LIBS_DIR}/lib/libyyjson.a"
                "-framework CoreFoundation"
                "-framework SystemConfiguration"
                "-framework Security"
            )
        else()
            target_link_libraries(tests PRIVATE
                "${SINDARIN_LIBS_DIR}/lib/libcurl.a"
                "${SINDARIN_LIBS_DIR}/lib/libssl.a"
                "${SINDARIN_LIBS_DIR}/lib/libcrypto.a"
                "${SINDARIN_LIBS_DIR}/lib/libz.a"
                "${SINDARIN_LIBS_DIR}/lib/libyyjson.a"
            )
        endif()
    else()
        target_compile_definitions(tests PRIVATE SN_HAS_CURL=0)
    endif()

    if(EXISTS "${SINDARIN_LIBS_DIR}/include/git2.h" AND EXISTS "${SINDARIN_LIBS_DIR}/include/yaml.h")
        target_compile_definitions(tests PRIVATE SN_HAS_PACKAGE_MANAGER=1)
        if(WIN32)
            target_link_libraries(tests PRIVATE
                "${SINDARIN_LIBS_DIR}/lib/libgit2.a"
                "${SINDARIN_LIBS_DIR}/lib/libyaml.a"
                "${SINDARIN_LIBS_DIR}/lib/libssh2.a"
                "${SINDARIN_LIBS_DIR}/lib/libpcre2-8.a"
                "${SINDARIN_LIBS_DIR}/lib/libhttp_parser.a"
            )
        elseif(APPLE)
            target_link_libraries(tests PRIVATE
                "${SINDARIN_LIBS_DIR}/lib/libgit2.a"
                "${SINDARIN_LIBS_DIR}/lib/libyaml.a"
                "${SINDARIN_LIBS_DIR}/lib/libssh2.a"
                "${SINDARIN_LIBS_DIR}/lib/libpcre2-8.a"
                "${SINDARIN_LIBS_DIR}/lib/libhttp_parser.a"
                iconv
            )
        else()
            target_link_libraries(tests PRIVATE
                "${SINDARIN_LIBS_DIR}/lib/libgit2.a"
                "${SINDARIN_LIBS_DIR}/lib/libyaml.a"
                "${SINDARIN_LIBS_DIR}/lib/libssh2.a"
                "${SINDARIN_LIBS_DIR}/lib/libpcre2-8.a"
                "${SINDARIN_LIBS_DIR}/lib/libhttp_parser.a"
            )
        endif()
    else()
        target_compile_definitions(tests PRIVATE SN_HAS_PACKAGE_MANAGER=0)
    endif()
else()
    message(STATUS "Dependencies not found in libs submodule")
    message(STATUS "  Run 'make setup' to initialize the libs submodule")
    target_compile_definitions(sn PRIVATE SN_HAS_CURL=0)
    target_compile_definitions(tests PRIVATE SN_HAS_CURL=0 SN_HAS_PACKAGE_MANAGER=0)
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

# Components for packaging (deps no longer bundled - users install via sn --install sindarin-libs)
set(CPACK_COMPONENTS_ALL compiler runtime headers configs)

set(CPACK_COMPONENT_COMPILER_DISPLAY_NAME "Sindarin Compiler")
set(CPACK_COMPONENT_COMPILER_REQUIRED ON)
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Runtime Libraries")
set(CPACK_COMPONENT_RUNTIME_DEPENDS compiler)
set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "Development Headers")
set(CPACK_COMPONENT_HEADERS_DEPENDS runtime)
set(CPACK_COMPONENT_CONFIGS_DISPLAY_NAME "Configuration Files")

# Platform-specific generators
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-windows-x64")
elseif(APPLE)
    set(CPACK_GENERATOR "TGZ")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-darwin-x64")
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
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.17), gcc, libc6-dev")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

    # RPM settings (only used if rpmbuild is available)
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Development/Languages")
    set(CPACK_RPM_PACKAGE_REQUIRES "gcc, glibc-devel")
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
endif()

# Source package
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
    "/[.]git/"
    "/build/"
    "/bin/"
    "/log/"
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
