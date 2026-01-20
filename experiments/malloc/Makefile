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
.PHONY: all build rebuild run clean help test-hooks

#------------------------------------------------------------------------------
# Platform Detection (copied from main Makefile)
#------------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
    PLATFORM := windows
    EXE_EXT := .exe
    PYTHON := python
else
    UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)
    ifneq ($(filter MINGW% MSYS% CYGWIN%,$(UNAME_S)),)
        PLATFORM := windows
        EXE_EXT := .exe
        PYTHON := python
    else ifeq ($(UNAME_S),Darwin)
        PLATFORM := darwin
        EXE_EXT :=
        PYTHON := python3
    else
        PLATFORM := linux
        EXE_EXT :=
        PYTHON := python3
    endif
endif

#------------------------------------------------------------------------------
# Directories
#------------------------------------------------------------------------------
# Experiment directories
# On Windows, Make's internal functions (wildcard, pattern matching) don't work
# with MSYS-style paths (/c/Users/...). Use cygpath -m to get Windows paths.
ifeq ($(PLATFORM),windows)
    EXP_DIR := $(shell cygpath -m "$$(pwd)")
    ROOT_DIR := $(shell cygpath -m "$$(realpath ../..)")
else
    EXP_DIR := $(shell pwd)
    ROOT_DIR := $(shell realpath ../..)
endif
EXP_SRC := $(EXP_DIR)/src
EXP_BIN := $(EXP_DIR)/bin
EXP_BUILD := $(EXP_DIR)/build

# Main project directories (relative to experiment)
ROOT_SRC := $(ROOT_DIR)/src
ROOT_SAMPLES := $(ROOT_DIR)/samples

# Temp directory for outputs
TEMP_DIR := /tmp

#------------------------------------------------------------------------------
# Compiler Configuration
#------------------------------------------------------------------------------
ifeq ($(PLATFORM),darwin)
    CC := clang
else ifeq ($(PLATFORM),windows)
    CC := clang
else
    CC := gcc
endif

# Common flags
CFLAGS := -Wall -Wextra -Wno-unknown-pragmas -O2 -std=c99 -D_GNU_SOURCE

# Enable memory hooks
CFLAGS += -DSN_MALLOC_HOOKS

# Include paths - experiment headers take priority
INCLUDES := -I$(EXP_SRC) -I$(EXP_SRC)/runtime -I$(ROOT_SRC) -I$(ROOT_SRC)/runtime -I$(ROOT_SRC)/platform

# Linker flags
ifeq ($(PLATFORM),windows)
    LDFLAGS := -lws2_32 -lbcrypt -lpthread
else
    LDFLAGS := -lpthread -lm
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
    $(ROOT_SRC)/runtime/runtime_string.c \
    $(ROOT_SRC)/runtime/runtime_array.c \
    $(ROOT_SRC)/runtime/runtime_io.c \
    $(ROOT_SRC)/runtime/runtime_byte.c \
    $(ROOT_SRC)/runtime/runtime_thread.c \
    $(ROOT_SRC)/runtime/runtime_any.c \
    $(ROOT_SRC)/runtime/runtime_intercept.c

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
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Compile experiment source files (takes priority for arena)
$(EXP_BUILD)/exp/%.o: $(EXP_SRC)/%.c | $(EXP_BUILD)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

#------------------------------------------------------------------------------
# Runtime Library Build (with hooks for compiled programs)
#------------------------------------------------------------------------------
$(RUNTIME_LIB): $(RUNTIME_LIB_OBJECTS) | $(RUNTIME_LIB_DIR)
	@echo "Creating hooked runtime library..."
	ar rcs $@ $^

# Compile runtime lib sources from root (without arena)
$(EXP_BUILD)/lib/%.o: $(ROOT_SRC)/%.c | $(EXP_BUILD)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Compile experiment runtime sources (hooked arena)
$(EXP_BUILD)/lib-exp/%.o: $(EXP_SRC)/%.c | $(EXP_BUILD)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(RUNTIME_LIB_DIR):
	@mkdir -p $@

#------------------------------------------------------------------------------
# Copy headers, config, and SDK for runtime
#------------------------------------------------------------------------------
.PHONY: copy-headers
copy-headers:
	@mkdir -p $(EXP_BIN)/include/runtime $(EXP_BIN)/include/platform $(EXP_BIN)/sdk
	@cp $(ROOT_SRC)/runtime.h $(EXP_BIN)/include/ 2>/dev/null || true
	@cp $(ROOT_SRC)/runtime/*.h $(EXP_BIN)/include/runtime/ 2>/dev/null || true
	@cp $(EXP_SRC)/runtime/*.h $(EXP_BIN)/include/runtime/ 2>/dev/null || true
	@cp $(ROOT_SRC)/platform/*.h $(EXP_BIN)/include/platform/ 2>/dev/null || true
	@cp $(ROOT_DIR)/etc/sn.cfg $(EXP_BIN)/sn.cfg 2>/dev/null || true
	@cp $(ROOT_DIR)/sdk/*.sn $(EXP_BIN)/sdk/ 2>/dev/null || true
	@cp $(ROOT_DIR)/sdk/*.sn.c $(EXP_BIN)/sdk/ 2>/dev/null || true

#------------------------------------------------------------------------------
# Directory creation
#------------------------------------------------------------------------------
$(EXP_BIN):
	@mkdir -p $@

$(EXP_BUILD):
	@mkdir -p $@

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
	timeout 30 $(SN) $(ROOT_SAMPLES)/main.sn -o $(TEMP_DIR)/hello-world-exp$(EXE_EXT) -l 3
	@echo ""
	@echo "Running compiled program..."
	@echo ""
	timeout 10 $(TEMP_DIR)/hello-world-exp$(EXE_EXT)

#------------------------------------------------------------------------------
# test-hooks - Run a simple test that shows hook output
#------------------------------------------------------------------------------
test-hooks: build
	@echo ""
	@echo "Testing memory hooks with a simple compilation..."
	@echo "You should see [SN_HOOK] messages below:"
	@echo ""
	@echo "--- Hook Output Start ---"
	timeout 30 $(SN) $(ROOT_SAMPLES)/main.sn -o $(TEMP_DIR)/test-hooks$(EXE_EXT) 2>&1 | head -50
	@echo "--- Hook Output End (truncated to 50 lines) ---"
	@echo ""
	@echo "Hook test complete. The [SN_HOOK] messages show memory allocations."

#------------------------------------------------------------------------------
# clean - Remove build artifacts
#------------------------------------------------------------------------------
clean:
	@echo "Cleaning experiment build artifacts..."
	rm -rf $(EXP_BUILD)
	rm -rf $(EXP_BIN)
	rm -f $(TEMP_DIR)/hello-world-exp$(EXE_EXT)
	rm -f $(TEMP_DIR)/test-hooks$(EXE_EXT)
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
	@echo ""
	@echo "Test Targets:"
	@echo "  make run          Compile and run samples/main.sn"
	@echo "  make test-hooks   Show hook output from compilation"
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
