# Sn Compiler - Source Makefile
# Builds the compiler, runtime libraries, and test binary

#------------------------------------------------------------------------------
# Phony targets
#------------------------------------------------------------------------------
.PHONY: all clean
.PHONY: runtime runtime-gcc runtime-clang runtime-tinycc runtime-all
.PHONY: tests tests-clang tests-tcc
.PHONY: create-bin-dir create-lib-dir copy-runtime-header create-symlink

#------------------------------------------------------------------------------
# Backend Configuration
#------------------------------------------------------------------------------
# Use BACKEND=gcc|clang|tinycc to select which backend to build
BACKEND ?= gcc

# Backend-specific settings
# Note: BINARY_SUFFIX is for the executable name (sn-gcc, sn-clang, sn-tcc)
#       LIB_SUFFIX is for the runtime lib directory (gcc, clang, tinycc)
ifeq ($(BACKEND),gcc)
    BACKEND_CC      := gcc
    BINARY_SUFFIX   := gcc
    LIB_SUFFIX      := gcc
    BACKEND_DEPFLAGS := -MMD -MP
else ifeq ($(BACKEND),clang)
    BACKEND_CC      := clang
    BINARY_SUFFIX   := clang
    LIB_SUFFIX      := clang
    BACKEND_DEPFLAGS := -MMD -MP
else ifeq ($(BACKEND),tinycc)
    BACKEND_CC      := tcc
    BINARY_SUFFIX   := tcc
    LIB_SUFFIX      := tinycc
    # TinyCC doesn't support -MMD/-MP
    BACKEND_DEPFLAGS :=
else
    $(error Unknown BACKEND: $(BACKEND). Use gcc, clang, or tinycc)
endif

#------------------------------------------------------------------------------
# Compiler Configuration
#------------------------------------------------------------------------------
# Compiler for building sn itself (always gcc)
CC := gcc

# Common flags
COMMON_CFLAGS := -Wall -Wextra -std=c99 -D_GNU_SOURCE -I. -Iruntime

# Platform detection for Windows-specific libraries
ifeq ($(OS),Windows_NT)
    PLATFORM_LIBS := -lws2_32 -lbcrypt
else
    PLATFORM_LIBS :=
endif

# Release build (default): optimized with LTO
ifndef DEBUG
    CFLAGS  := $(COMMON_CFLAGS) -O3 -flto -MMD -MP
    LDFLAGS := -flto -O3 -lpthread $(PLATFORM_LIBS)
else
    # Debug build: use 'make DEBUG=1' for sanitizer and debug symbols
    CFLAGS  := $(COMMON_CFLAGS) -g -fsanitize=address -fno-omit-frame-pointer -MMD -MP
    LDFLAGS := -fsanitize=address -lpthread -lm $(PLATFORM_LIBS)
endif

# Runtime compilation flags (no LTO - must link in both release and debug modes)
RUNTIME_CFLAGS := -Wall -std=c99 -O2 $(BACKEND_DEPFLAGS) -D_GNU_SOURCE -I. -Iruntime

#------------------------------------------------------------------------------
# Directories
#------------------------------------------------------------------------------
BIN_DIR     := ../bin
INCLUDE_DIR := $(BIN_DIR)/include
LIB_DIR     := $(BIN_DIR)/lib/$(LIB_SUFFIX)
TEST_SRCDIR := ../tests/unit

#------------------------------------------------------------------------------
# Source Files (organized by module)
#------------------------------------------------------------------------------

# Core compiler sources
CORE_SRCS := \
    arena.c \
    file.c \
    runtime.c \
    token.c \
    compiler.c \
    debug.c \
    diagnostic.c \
    gcc_backend.c \
    main.c

# Lexer module
LEXER_SRCS := \
    lexer.c \
    lexer/lexer_util.c \
    lexer/lexer_scan.c

# AST module
AST_SRCS := \
    ast.c \
    ast/ast_type.c \
    ast/ast_expr.c \
    ast/ast_stmt.c \
    ast/ast_print.c

# Parser module
PARSER_SRCS := \
    parser.c \
    parser/parser_util.c \
    parser/parser_expr.c \
    parser/parser_stmt.c \
    parser/parser_stmt_decl.c \
    parser/parser_stmt_control.c

# Symbol table module
SYMBOL_TABLE_SRCS := \
    symbol_table.c \
    symbol_table/symbol_table_core.c \
    symbol_table/symbol_table_namespace.c \
    symbol_table/symbol_table_thread.c

# Type checker module
TYPE_CHECKER_SRCS := \
    type_checker.c \
    type_checker/type_checker_util.c \
    type_checker/type_checker_expr.c \
    type_checker/type_checker_expr_call.c \
    type_checker/type_checker_expr_call_core.c \
    type_checker/type_checker_expr_call_array.c \
    type_checker/type_checker_expr_call_string.c \
    type_checker/type_checker_expr_call_file.c \
    type_checker/type_checker_expr_call_time.c \
    type_checker/type_checker_expr_array.c \
    type_checker/type_checker_expr_lambda.c \
    type_checker/type_checker_stmt.c

# Code generator module
CODE_GEN_SRCS := \
    code_gen.c \
    code_gen/code_gen_util.c \
    code_gen/code_gen_expr.c \
    code_gen/code_gen_expr_core.c \
    code_gen/code_gen_expr_binary.c \
    code_gen/code_gen_expr_lambda.c \
    code_gen/code_gen_expr_call.c \
    code_gen/code_gen_expr_call_array.c \
    code_gen/code_gen_expr_call_file.c \
    code_gen/code_gen_expr_call_string.c \
    code_gen/code_gen_expr_call_time.c \
    code_gen/code_gen_expr_array.c \
    code_gen/code_gen_expr_static.c \
    code_gen/code_gen_expr_string.c \
    code_gen/code_gen_expr_thread.c \
    code_gen/code_gen_stmt_core.c \
    code_gen/code_gen_stmt_loop.c \
    code_gen/code_gen_stmt_capture.c

# Optimizer module
OPTIMIZER_SRCS := \
    optimizer.c \
    optimizer/optimizer_util.c \
    optimizer/optimizer_tail_call.c \
    optimizer/optimizer_string.c

# Runtime module (compiled into compiler and as separate library)
RUNTIME_MODULE_SRCS := \
    runtime/runtime_arena.c \
    runtime/runtime_string.c \
    runtime/runtime_array.c \
    runtime/runtime_text_file.c \
    runtime/runtime_binary_file.c \
    runtime/runtime_io.c \
    runtime/runtime_byte.c \
    runtime/runtime_path.c \
    runtime/runtime_date.c \
    runtime/runtime_time.c \
    runtime/runtime_thread.c \
    runtime/runtime_any.c \
    runtime/runtime_intercept.c

# All compiler sources
SRCS := $(CORE_SRCS) $(LEXER_SRCS) $(AST_SRCS) $(PARSER_SRCS) \
        $(SYMBOL_TABLE_SRCS) $(TYPE_CHECKER_SRCS) $(CODE_GEN_SRCS) \
        $(OPTIMIZER_SRCS) $(RUNTIME_MODULE_SRCS)

# Runtime library sources (subset for standalone runtime)
RUNTIME_SRCS := arena.c debug.c runtime.c $(RUNTIME_MODULE_SRCS)

#------------------------------------------------------------------------------
# Runtime Headers (for copying to include dir)
#------------------------------------------------------------------------------
RUNTIME_HEADERS := \
    runtime.h \
    runtime/runtime_atomic_compat.h \
    runtime/runtime_arena.h \
    runtime/runtime_string.h \
    runtime/runtime_array.h \
    runtime/runtime_file.h \
    runtime/runtime_io.h \
    runtime/runtime_byte.h \
    runtime/runtime_path.h \
    runtime/runtime_date.h \
    runtime/runtime_time.h \
    runtime/runtime_thread.h \
    runtime/runtime_any.h \
    runtime/runtime_intercept.h

#------------------------------------------------------------------------------
# Object Files
#------------------------------------------------------------------------------
OBJS := $(addprefix $(BIN_DIR)/, $(notdir $(SRCS:.c=.o)))
DEPS := $(OBJS:.o=.d)
RUNTIME_OBJS := $(addprefix $(LIB_DIR)/, $(notdir $(RUNTIME_SRCS:.c=.o)))

# Target binary
TARGET := $(BIN_DIR)/sn-$(BINARY_SUFFIX)

# VPATH for source file lookup
VPATH := . ast lexer parser type_checker optimizer code_gen runtime symbol_table

#------------------------------------------------------------------------------
# Main Build Targets
#------------------------------------------------------------------------------
all: create-bin-dir $(TARGET) copy-runtime-header runtime create-symlink

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	@chmod +x $@

$(BIN_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

#------------------------------------------------------------------------------
# Symlink Creation
#------------------------------------------------------------------------------
create-symlink:
	@rm -f $(BIN_DIR)/sn
	@ln -s $(notdir $(TARGET)) $(BIN_DIR)/sn
	@echo "Created symlink: sn -> $(notdir $(TARGET))"

create-bin-dir:
	@mkdir -p $(BIN_DIR)

#------------------------------------------------------------------------------
# Runtime Header Installation
#------------------------------------------------------------------------------
copy-runtime-header: create-bin-dir
	@mkdir -p $(INCLUDE_DIR)/runtime
	@for header in $(RUNTIME_HEADERS); do \
		cp $$header $(INCLUDE_DIR)/$$header; \
	done

#------------------------------------------------------------------------------
# Runtime Library Builds
#------------------------------------------------------------------------------
runtime: create-lib-dir $(RUNTIME_OBJS)
	@find $(LIB_DIR) -name "*.d" -delete 2>/dev/null || true
	@echo "Built runtime for $(BACKEND) backend in $(LIB_DIR)"

create-lib-dir:
	@mkdir -p $(LIB_DIR)

# Runtime object compilation rules
$(LIB_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(BACKEND_CC) $(RUNTIME_CFLAGS) -c $< -o $@

$(LIB_DIR)/%.o: runtime/%.c
	@mkdir -p $(@D)
	$(BACKEND_CC) $(RUNTIME_CFLAGS) -c $< -o $@

# Convenience targets for each backend
runtime-gcc:
	@$(MAKE) runtime BACKEND=gcc

runtime-clang:
	@$(MAKE) runtime BACKEND=clang

runtime-tinycc:
	@$(MAKE) runtime BACKEND=tinycc

runtime-all: runtime-gcc runtime-clang runtime-tinycc
	@echo "Built runtime for all backends"

#------------------------------------------------------------------------------
# Test Build
#------------------------------------------------------------------------------
TEST_SRCS_FILES := $(TEST_SRCDIR)/_all_.c
TEST_OBJS := $(addprefix $(BIN_DIR)/, $(notdir $(TEST_SRCS_FILES:.c=.o)))
TEST_TARGET := $(BIN_DIR)/tests

# Compiler objects needed for tests (everything except main.o)
TEST_COMPILER_OBJS := $(filter-out $(BIN_DIR)/main.o, \
    $(addprefix $(BIN_DIR)/, $(notdir $(CORE_SRCS:.c=.o))) \
    $(addprefix $(BIN_DIR)/, $(notdir $(LEXER_SRCS:.c=.o))) \
    $(addprefix $(BIN_DIR)/, $(notdir $(AST_SRCS:.c=.o))) \
    $(addprefix $(BIN_DIR)/, $(notdir $(PARSER_SRCS:.c=.o))) \
    $(addprefix $(BIN_DIR)/, $(notdir $(SYMBOL_TABLE_SRCS:.c=.o))) \
    $(addprefix $(BIN_DIR)/, $(notdir $(TYPE_CHECKER_SRCS:.c=.o))) \
    $(addprefix $(BIN_DIR)/, $(notdir $(CODE_GEN_SRCS:.c=.o))) \
    $(addprefix $(BIN_DIR)/, $(notdir $(OPTIMIZER_SRCS:.c=.o))))

# Runtime objects for tests - generate from RUNTIME_MODULE_SRCS
RUNTIME_OBJ_NAMES := runtime.o runtime_arena.o runtime_string.o runtime_array.o \
    runtime_text_file.o runtime_binary_file.o runtime_io.o runtime_byte.o \
    runtime_path.o runtime_date.o runtime_time.o runtime_thread.o \
    runtime_any.o runtime_intercept.o

TEST_RUNTIME_GCC   := $(addprefix $(BIN_DIR)/, $(RUNTIME_OBJ_NAMES))
TEST_RUNTIME_CLANG := $(addprefix $(BIN_DIR)/lib/clang/, $(RUNTIME_OBJ_NAMES))
TEST_RUNTIME_TCC   := $(addprefix $(BIN_DIR)/lib/tinycc/, $(RUNTIME_OBJ_NAMES))

# Default test target (GCC runtime)
tests: create-bin-dir $(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS) $(TEST_COMPILER_OBJS) $(TEST_RUNTIME_GCC)
	$(CC) -o $@ $^ $(LDFLAGS) -lm

# Test binary with Clang runtime
tests-clang: create-bin-dir $(BIN_DIR)/tests-clang

$(BIN_DIR)/tests-clang: $(TEST_OBJS) $(TEST_COMPILER_OBJS) $(TEST_RUNTIME_CLANG)
	$(CC) -o $@ $^ -lpthread -lm

# Test binary with TinyCC runtime
tests-tcc: create-bin-dir $(BIN_DIR)/tests-tcc

$(BIN_DIR)/tests-tcc: $(TEST_OBJS) $(TEST_COMPILER_OBJS) $(TEST_RUNTIME_TCC)
	$(CC) -o $@ $^ -lpthread -lm

# Test source compilation
$(BIN_DIR)/%.o: $(TEST_SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

#------------------------------------------------------------------------------
# Clean
#------------------------------------------------------------------------------
clean:
	@rm -f $(BIN_DIR)/sn $(BIN_DIR)/sn-* $(BIN_DIR)/tests $(BIN_DIR)/tests-*
	@rm -f $(BIN_DIR)/*.o $(BIN_DIR)/*.d
	@rm -rf $(BIN_DIR)/lib $(BIN_DIR)/include
    