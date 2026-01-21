# Malloc Hooks Experiment - Makefile
#
# This Makefile builds a modified compiler with memory allocation hooks enabled.
# It uses the main project's source files but substitutes the experimental
# arena implementation that includes hook output.
#
# See README.md for experiment details and platform-specific hook documentation.

#------------------------------------------------------------------------------
# Phony targets
#------------------------------------------------------------------------------
.PHONY: all build rebuild run clean help test test-redirect copy-headers setup-deps

#------------------------------------------------------------------------------
# Platform Detection (copied from main Makefile)
#------------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
    PLATFORM := windows
    EXE_EXT := .exe
    PYTHON := python
    TEMP_DIR := $(if $(TEMP),$(TEMP),/tmp)
else
    UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)
    ifneq ($(filter MINGW% MSYS% CYGWIN%,$(UNAME_S)),)
        PLATFORM := windows
        EXE_EXT := .exe
        PYTHON := python
        TEMP_DIR := /tmp
    else ifeq ($(UNAME_S),Darwin)
        PLATFORM := darwin
        EXE_EXT :=
        PYTHON := python3
        TEMP_DIR := /tmp
    else
        PLATFORM := linux
        EXE_EXT :=
        PYTHON := python3
        TEMP_DIR := /tmp
    endif
endif

#------------------------------------------------------------------------------
# Directories
#------------------------------------------------------------------------------
# Use Make built-ins for portable paths
EXP_DIR := $(CURDIR)
ROOT_DIR := $(abspath $(EXP_DIR)/../..)
EXP_SRC := $(EXP_DIR)/src
EXP_BIN := $(EXP_DIR)/bin
EXP_BUILD := $(EXP_DIR)/build

# Main project directories
ROOT_SRC := $(ROOT_DIR)/src
ROOT_SAMPLES := $(ROOT_DIR)/samples

# vcpkg location (in project root)
VCPKG_ROOT := $(ROOT_DIR)/vcpkg

#------------------------------------------------------------------------------
# Compiler Configuration
#------------------------------------------------------------------------------
ifeq ($(PLATFORM),darwin)
    CC := clang
    TIMEOUT := timeout 30
else ifeq ($(PLATFORM),windows)
    CC := clang
    TIMEOUT :=
else
    CC := gcc
    TIMEOUT := timeout 30
endif

# Common flags
CFLAGS := -Wall -Wextra -Wno-unknown-pragmas -O2 -std=gnu99 -D_GNU_SOURCE

# Enable memory hooks
CFLAGS += -DSN_MALLOC_HOOKS

# Include paths - experiment headers take priority
INCLUDES := -I$(EXP_SRC) -I$(EXP_SRC)/runtime -I$(ROOT_SRC) -I$(ROOT_SRC)/runtime -I$(ROOT_SRC)/platform

# Windows: Add MinHook include path
ifeq ($(PLATFORM),windows)
    INCLUDES += -I$(EXP_SRC)/runtime/minhook -I$(EXP_SRC)/runtime/minhook/hde
endif

# Linker flags
ifeq ($(PLATFORM),windows)
    LDFLAGS := -lws2_32 -lbcrypt -lpthread -ldbghelp
else ifeq ($(PLATFORM),darwin)
    LDFLAGS := -lpthread -lm
else
    LDFLAGS := -lpthread -lm -ldl
endif

#------------------------------------------------------------------------------
# Source Files
#------------------------------------------------------------------------------
# Core sources from main project
CORE_SOURCES := \
    $(ROOT_SRC)/arena.c \
    $(ROOT_SRC)/file.c \
    $(ROOT_SRC)/token.c \
    $(ROOT_SRC)/debug.c \
    $(ROOT_SRC)/diagnostic.c \
    $(ROOT_SRC)/compiler.c

# Lexer sources
LEXER_SOURCES := \
    $(ROOT_SRC)/lexer.c \
    $(ROOT_SRC)/lexer/lexer_util.c \
    $(ROOT_SRC)/lexer/lexer_scan.c

# AST sources
AST_SOURCES := \
    $(ROOT_SRC)/ast.c \
    $(ROOT_SRC)/ast/ast_type.c \
    $(ROOT_SRC)/ast/ast_expr.c \
    $(ROOT_SRC)/ast/ast_stmt.c \
    $(ROOT_SRC)/ast/ast_print.c

# Parser sources
PARSER_SOURCES := \
    $(ROOT_SRC)/parser.c \
    $(ROOT_SRC)/parser/parser_util.c \
    $(ROOT_SRC)/parser/parser_expr.c \
    $(ROOT_SRC)/parser/parser_stmt.c \
    $(ROOT_SRC)/parser/parser_stmt_decl.c \
    $(ROOT_SRC)/parser/parser_stmt_control.c

# Symbol table sources
SYMBOL_TABLE_SOURCES := \
    $(ROOT_SRC)/symbol_table.c \
    $(ROOT_SRC)/symbol_table/symbol_table_core.c \
    $(ROOT_SRC)/symbol_table/symbol_table_namespace.c \
    $(ROOT_SRC)/symbol_table/symbol_table_thread.c

# Type checker sources
TYPE_CHECKER_SOURCES := \
    $(ROOT_SRC)/type_checker.c \
    $(ROOT_SRC)/type_checker/type_checker_util.c \
    $(ROOT_SRC)/type_checker/type_checker_expr.c \
    $(ROOT_SRC)/type_checker/type_checker_expr_call.c \
    $(ROOT_SRC)/type_checker/type_checker_expr_call_core.c \
    $(ROOT_SRC)/type_checker/type_checker_expr_call_array.c \
    $(ROOT_SRC)/type_checker/type_checker_expr_call_string.c \
    $(ROOT_SRC)/type_checker/type_checker_expr_array.c \
    $(ROOT_SRC)/type_checker/type_checker_expr_lambda.c \
    $(ROOT_SRC)/type_checker/type_checker_stmt.c

# Optimizer sources
OPTIMIZER_SOURCES := \
    $(ROOT_SRC)/optimizer.c \
    $(ROOT_SRC)/optimizer/optimizer_util.c \
    $(ROOT_SRC)/optimizer/optimizer_tail_call.c \
    $(ROOT_SRC)/optimizer/optimizer_string.c

# Code generator sources
CODEGEN_SOURCES := \
    $(ROOT_SRC)/code_gen.c \
    $(ROOT_SRC)/code_gen/code_gen_util.c \
    $(ROOT_SRC)/code_gen/code_gen_expr.c \
    $(ROOT_SRC)/code_gen/code_gen_expr_core.c \
    $(ROOT_SRC)/code_gen/code_gen_expr_binary.c \
    $(ROOT_SRC)/code_gen/code_gen_expr_lambda.c \
    $(ROOT_SRC)/code_gen/code_gen_expr_call.c \
    $(ROOT_SRC)/code_gen/code_gen_expr_call_array.c \
    $(ROOT_SRC)/code_gen/code_gen_expr_call_string.c \
    $(ROOT_SRC)/code_gen/code_gen_expr_array.c \
    $(ROOT_SRC)/code_gen/code_gen_expr_static.c \
    $(ROOT_SRC)/code_gen/code_gen_expr_string.c \
    $(ROOT_SRC)/code_gen/code_gen_expr_thread.c \
    $(ROOT_SRC)/code_gen/code_gen_stmt_core.c \
    $(ROOT_SRC)/code_gen/code_gen_stmt_loop.c \
    $(ROOT_SRC)/code_gen/code_gen_stmt_capture.c

# Backend sources
BACKEND_SOURCES := \
    $(ROOT_SRC)/gcc_backend.c

# Runtime sources from main project (EXCEPT runtime_arena.c which we override)
RUNTIME_SOURCES := \
    $(ROOT_SRC)/runtime.c \
    $(ROOT_SRC)/runtime/runtime_string.c \
    $(ROOT_SRC)/runtime/runtime_array.c \
    $(ROOT_SRC)/runtime/runtime_io.c \
    $(ROOT_SRC)/runtime/runtime_byte.c \
    $(ROOT_SRC)/runtime/runtime_thread.c \
    $(ROOT_SRC)/runtime/runtime_any.c \
    $(ROOT_SRC)/runtime/runtime_intercept.c

# Experimental arena source (overrides the main one)
EXP_ARENA_SOURCE := $(EXP_SRC)/runtime/runtime_arena.c

# Main entry point
MAIN_SOURCE := $(ROOT_SRC)/main.c

#------------------------------------------------------------------------------
# Runtime Library Sources (for compiled programs)
#------------------------------------------------------------------------------
RUNTIME_LIB_SOURCES := \
    $(ROOT_SRC)/arena.c \
    $(ROOT_SRC)/debug.c \
    $(ROOT_SRC)/runtime.c \
    $(EXP_SRC)/runtime/runtime_arena.c \
    $(EXP_SRC)/runtime/runtime_malloc_hooks.c \
    $(EXP_SRC)/runtime/runtime_malloc_redirect.c \
    $(ROOT_SRC)/runtime/runtime_string.c \
    $(ROOT_SRC)/runtime/runtime_array.c \
    $(ROOT_SRC)/runtime/runtime_io.c \
    $(ROOT_SRC)/runtime/runtime_byte.c \
    $(ROOT_SRC)/runtime/runtime_thread.c \
    $(ROOT_SRC)/runtime/runtime_any.c \
    $(ROOT_SRC)/runtime/runtime_intercept.c

# Platform-specific hooking libraries for runtime symbol interception
ifeq ($(PLATFORM),darwin)
    # macOS: fishhook for runtime symbol rebinding
    RUNTIME_LIB_SOURCES += $(EXP_SRC)/runtime/fishhook.c
else ifeq ($(PLATFORM),linux)
    # Linux: plthook for PLT/GOT modification
    RUNTIME_LIB_SOURCES += $(EXP_SRC)/runtime/plthook_elf.c
else ifeq ($(PLATFORM),windows)
    # Windows: MinHook for inline function hooking
    RUNTIME_LIB_SOURCES += \
        $(EXP_SRC)/runtime/minhook/hook.c \
        $(EXP_SRC)/runtime/minhook/buffer.c \
        $(EXP_SRC)/runtime/minhook/trampoline.c \
        $(EXP_SRC)/runtime/minhook/hde/hde32.c \
        $(EXP_SRC)/runtime/minhook/hde/hde64.c
endif

RUNTIME_LIB_OBJECTS := $(patsubst $(ROOT_SRC)/%.c,$(EXP_BUILD)/lib/%.o,$(filter $(ROOT_SRC)/%,$(RUNTIME_LIB_SOURCES)))
RUNTIME_LIB_OBJECTS += $(patsubst $(EXP_SRC)/%.c,$(EXP_BUILD)/lib-exp/%.o,$(filter $(EXP_SRC)/%,$(RUNTIME_LIB_SOURCES)))

# All sources combined
ALL_SOURCES := \
    $(MAIN_SOURCE) \
    $(CORE_SOURCES) \
    $(LEXER_SOURCES) \
    $(AST_SOURCES) \
    $(PARSER_SOURCES) \
    $(SYMBOL_TABLE_SOURCES) \
    $(TYPE_CHECKER_SOURCES) \
    $(OPTIMIZER_SOURCES) \
    $(CODEGEN_SOURCES) \
    $(BACKEND_SOURCES) \
    $(RUNTIME_SOURCES) \
    $(EXP_ARENA_SOURCE)

#------------------------------------------------------------------------------
# Object Files
#------------------------------------------------------------------------------
# Convert source paths to object paths in build directory
# This is complex because sources come from different directories
OBJECTS := $(patsubst $(ROOT_SRC)/%.c,$(EXP_BUILD)/root/%.o,$(filter $(ROOT_SRC)/%,$(ALL_SOURCES)))
OBJECTS += $(patsubst $(EXP_SRC)/%.c,$(EXP_BUILD)/exp/%.o,$(filter $(EXP_SRC)/%,$(ALL_SOURCES)))

#------------------------------------------------------------------------------
# Output
#------------------------------------------------------------------------------
SN := $(EXP_BIN)/sn$(EXE_EXT)
ifeq ($(PLATFORM),windows)
    RUNTIME_LIB_DIR := $(EXP_BIN)/lib/clang
else
    RUNTIME_LIB_DIR := $(EXP_BIN)/lib/gcc
endif
RUNTIME_LIB := $(RUNTIME_LIB_DIR)/libsn_runtime.a

#------------------------------------------------------------------------------
# Default target
#------------------------------------------------------------------------------
all: build

#------------------------------------------------------------------------------
# build - Build the experimental compiler and runtime library
#------------------------------------------------------------------------------
build: $(SN) $(RUNTIME_LIB) copy-headers
	@echo ""
	@echo "=========================================="
	@echo "Malloc Hooks Experiment Build Complete!"
	@echo "=========================================="
	@echo "Compiler: $(SN)"
	@echo "Runtime:  $(RUNTIME_LIB)"
	@echo "Hooks: ENABLED (SN_MALLOC_HOOKS defined)"
	@echo ""
	@echo "Run 'make run' to test with samples/main.sn"
	@echo "Run 'make test-hooks' to see hook output"
	@echo ""

$(SN): $(OBJECTS) | $(EXP_BIN)
	@echo "Linking experimental compiler..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile root source files
$(EXP_BUILD)/root/%.o: $(ROOT_SRC)/%.c | $(EXP_BUILD)
	@cmake -E make_directory $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Compile experiment source files (takes priority for arena)
$(EXP_BUILD)/exp/%.o: $(EXP_SRC)/%.c | $(EXP_BUILD)
	@cmake -E make_directory $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

#------------------------------------------------------------------------------
# Runtime Library Build (with hooks for compiled programs)
#------------------------------------------------------------------------------
$(RUNTIME_LIB): $(RUNTIME_LIB_OBJECTS) | $(RUNTIME_LIB_DIR)
	@echo "Creating hooked runtime library..."
	ar rcs $@ $^

# Compile runtime lib sources from root (without arena)
$(EXP_BUILD)/lib/%.o: $(ROOT_SRC)/%.c | $(EXP_BUILD)
	@cmake -E make_directory $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Compile experiment runtime sources (hooked arena)
$(EXP_BUILD)/lib-exp/%.o: $(EXP_SRC)/%.c | $(EXP_BUILD)
	@cmake -E make_directory $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(RUNTIME_LIB_DIR):
	@cmake -E make_directory $@

#------------------------------------------------------------------------------
# Copy headers, config, and SDK for runtime
#------------------------------------------------------------------------------
copy-headers:
	@cmake -E make_directory $(EXP_BIN)/include/runtime
	@cmake -E make_directory $(EXP_BIN)/include/platform
	@cmake -E make_directory $(EXP_BIN)/sdk
	-@cmake -E copy $(ROOT_SRC)/runtime.h $(EXP_BIN)/include/
	-@cmake -E copy $(ROOT_SRC)/runtime/runtime_arena.h $(EXP_BIN)/include/runtime/
	-@cmake -E copy $(ROOT_SRC)/runtime/runtime_string.h $(EXP_BIN)/include/runtime/
	-@cmake -E copy $(ROOT_SRC)/runtime/runtime_array.h $(EXP_BIN)/include/runtime/
	-@cmake -E copy $(ROOT_SRC)/runtime/runtime_io.h $(EXP_BIN)/include/runtime/
	-@cmake -E copy $(ROOT_SRC)/runtime/runtime_byte.h $(EXP_BIN)/include/runtime/
	-@cmake -E copy $(ROOT_SRC)/runtime/runtime_thread.h $(EXP_BIN)/include/runtime/
	-@cmake -E copy $(ROOT_SRC)/runtime/runtime_any.h $(EXP_BIN)/include/runtime/
	-@cmake -E copy $(ROOT_SRC)/runtime/runtime_intercept.h $(EXP_BIN)/include/runtime/
	-@cmake -E copy $(ROOT_SRC)/runtime/runtime_atomic_compat.h $(EXP_BIN)/include/runtime/
	-@cmake -E copy $(ROOT_SRC)/runtime/runtime_internal.h $(EXP_BIN)/include/runtime/
	-@cmake -E copy $(EXP_SRC)/runtime/runtime_arena.h $(EXP_BIN)/include/runtime/
	-@cmake -E copy $(ROOT_SRC)/platform/platform.h $(EXP_BIN)/include/platform/
	-@cmake -E copy $(ROOT_SRC)/platform/compat_pthread.h $(EXP_BIN)/include/platform/
	-@cmake -E copy $(ROOT_SRC)/platform/compat_dirent.h $(EXP_BIN)/include/platform/
	-@cmake -E copy $(ROOT_SRC)/platform/compat_io.h $(EXP_BIN)/include/platform/
	-@cmake -E copy $(ROOT_SRC)/platform/compat_net.h $(EXP_BIN)/include/platform/
	-@cmake -E copy $(ROOT_SRC)/platform/compat_time.h $(EXP_BIN)/include/platform/
	-@cmake -E copy $(ROOT_SRC)/platform/compat_windows.h $(EXP_BIN)/include/platform/
	-@cmake -E copy $(EXP_DIR)/etc/sn.cfg $(EXP_BIN)/sn.cfg

#------------------------------------------------------------------------------
# Directory creation
#------------------------------------------------------------------------------
$(EXP_BIN):
	@cmake -E make_directory $@

$(EXP_BUILD):
	@cmake -E make_directory $@

#------------------------------------------------------------------------------
# rebuild - Clean and build
#------------------------------------------------------------------------------
rebuild: clean build

#------------------------------------------------------------------------------
# run - Compile and run samples/main.sn
#------------------------------------------------------------------------------
run: build
	@echo ""
	@echo "Compiling samples/main.sn with experimental compiler..."
	@echo ""
	$(SN) $(ROOT_SAMPLES)/main.sn -o $(TEMP_DIR)/hello-world-exp$(EXE_EXT) -l 3
	@echo ""
	@echo "Running compiled program..."
	@echo ""
	$(TEMP_DIR)/hello-world-exp$(EXE_EXT)

#------------------------------------------------------------------------------
# test - Compile and run test_zlib.sn
#------------------------------------------------------------------------------
export VCPKG_ROOT
# Debug symbols for symbol resolution in malloc hooks
# On Windows, use CodeView format for DbgHelp compatibility
# Use ?= to allow environment variables to override (e.g., for zlib paths on macOS)
ifeq ($(PLATFORM),windows)
    SN_CFLAGS ?= -g -gcodeview
else
    SN_CFLAGS ?= -g
endif

# Runtime hooking - no --wrap linker flags needed anymore!
# All platforms use runtime hooking libraries (plthook, fishhook, MinHook)
ifeq ($(PLATFORM),windows)
    # Windows: DbgHelp for symbol resolution, PDB for debug info
    SN_LDFLAGS ?= -ldbghelp -Wl,-pdb=
else ifeq ($(PLATFORM),darwin)
    # macOS: fishhook handles symbol rebinding at runtime
    # Allow environment to set library paths (e.g., for Homebrew zlib)
    SN_LDFLAGS ?=
else
    # Linux: -rdynamic exports symbols for dladdr to resolve function names
    # plthook handles malloc interception at runtime
    SN_LDFLAGS ?= -rdynamic
endif

# Export the variables to child processes (e.g., the compiler)
export SN_CFLAGS
export SN_LDFLAGS

test: build
	@echo ""
	@echo "Compiling tests/test_zlib.sn..."
	$(SN) $(EXP_DIR)/tests/test_zlib.sn -o $(TEMP_DIR)/test_zlib$(EXE_EXT)
	@echo ""
	@echo "Running test_zlib..."
	$(TEMP_DIR)/test_zlib$(EXE_EXT)

#------------------------------------------------------------------------------
# test-redirect - Test malloc redirect system
#------------------------------------------------------------------------------
TEST_REDIRECT_SOURCES := \
    $(EXP_DIR)/tests/test_malloc_redirect.c \
    $(EXP_SRC)/runtime/runtime_arena.c \
    $(EXP_SRC)/runtime/runtime_malloc_redirect.c

# Add platform-specific hooking library
ifeq ($(PLATFORM),darwin)
    TEST_REDIRECT_SOURCES += $(EXP_SRC)/runtime/fishhook.c
else ifeq ($(PLATFORM),linux)
    TEST_REDIRECT_SOURCES += $(EXP_SRC)/runtime/plthook_elf.c
else ifeq ($(PLATFORM),windows)
    TEST_REDIRECT_SOURCES += \
        $(EXP_SRC)/runtime/minhook/hook.c \
        $(EXP_SRC)/runtime/minhook/buffer.c \
        $(EXP_SRC)/runtime/minhook/trampoline.c \
        $(EXP_SRC)/runtime/minhook/hde/hde32.c \
        $(EXP_SRC)/runtime/minhook/hde/hde64.c
endif

TEST_REDIRECT_BIN := $(TEMP_DIR)/test_malloc_redirect$(EXE_EXT)

test-redirect: $(TEST_REDIRECT_BIN)
	@echo ""
	@echo "Running malloc redirect tests..."
	@echo ""
	$(TIMEOUT) $(TEST_REDIRECT_BIN)

$(TEST_REDIRECT_BIN): $(TEST_REDIRECT_SOURCES) | $(EXP_BIN)
	@echo ""
	@echo "Compiling malloc redirect test..."
	$(CC) -Wall -Wextra -Wno-unknown-pragmas -O0 -g -std=gnu99 -D_GNU_SOURCE -DSN_MALLOC_HOOKS -DSN_MALLOC_REDIRECT $(INCLUDES) -o $@ $(TEST_REDIRECT_SOURCES) $(LDFLAGS)

#------------------------------------------------------------------------------
# setup-deps - Install dependencies (platform-specific)
#------------------------------------------------------------------------------
setup-deps:
	@echo "Setting up dependencies..."
	$(PYTHON) $(ROOT_DIR)/scripts/setup_deps.py --vcpkg-root $(VCPKG_ROOT)
ifeq ($(PLATFORM),windows)
	@echo "Installing zlib via vcpkg (classic mode)..."
	$(VCPKG_ROOT)/vcpkg install zlib:x64-windows --classic
	@echo "Creating libz.a compatibility link..."
	-cmake -E copy $(VCPKG_ROOT)/installed/x64-windows/lib/zlib.lib $(VCPKG_ROOT)/installed/x64-windows/lib/libz.a
endif
	@echo "Dependencies ready."

#------------------------------------------------------------------------------
# clean - Remove build artifacts (using cmake -E for cross-platform)
#------------------------------------------------------------------------------
clean:
	@echo "Cleaning experiment build artifacts..."
	-cmake -E rm -rf $(EXP_BUILD)
	-cmake -E rm -rf $(EXP_BIN)
	-cmake -E rm -f $(TEMP_DIR)/hello-world-exp$(EXE_EXT)
	-cmake -E rm -f $(TEMP_DIR)/test_zlib$(EXE_EXT)
	@echo "Clean complete."

#------------------------------------------------------------------------------
# help - Show available targets
#------------------------------------------------------------------------------
help:
	@echo "Malloc Hooks Experiment - Build System"
	@echo ""
	@echo "This experiment builds a compiler with memory allocation hooks enabled."
	@echo "All malloc/calloc/free calls in the arena are traced."
	@echo ""
	@echo "Build Targets:"
	@echo "  make build        Build experimental compiler to bin/sn"
	@echo "  make rebuild      Clean and build"
	@echo "  make clean        Remove build artifacts"
	@echo "  make setup-deps   Install dependencies (vcpkg + zlib)"
	@echo ""
	@echo "Test Targets:"
	@echo "  make run          Compile and run samples/main.sn"
	@echo "  make test         Compile and run tests/test_zlib.sn"
	@echo ""
	@echo "Info:"
	@echo "  make help         Show this help message"
	@echo ""
	@echo "Configuration:"
	@echo "  Platform:    $(PLATFORM)"
	@echo "  Compiler:    $(CC)"
	@echo "  Output:      $(SN)"
	@echo "  Hooks:       ENABLED (SN_MALLOC_HOOKS)"
	@echo ""
	@echo "See README.md for platform-specific hook documentation."
