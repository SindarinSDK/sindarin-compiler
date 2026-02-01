# Sn Compiler - Makefile (CMake Wrapper)
#
# This Makefile provides familiar Make targets that delegate to CMake.
# For advanced usage, use CMake directly with presets:
#   cmake --preset linux-gcc-release
#   cmake --build --preset linux-gcc-release
#
# See CMakePresets.json for all available presets.

#------------------------------------------------------------------------------
# Phony targets
#------------------------------------------------------------------------------
.PHONY: all build rebuild run clean test help
.PHONY: test-unit test-cgen test-integration test-integration-errors
.PHONY: test-explore test-explore-errors
.PHONY: arena test-arena
.PHONY: configure install package docs
.PHONY: setup libs

#------------------------------------------------------------------------------
# Platform Detection
#------------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
    PLATFORM := windows
    CMAKE_PRESET := windows-clang-release
    CMAKE_DEBUG_PRESET := windows-clang-debug
    EXE_EXT := .exe
    PYTHON := python
    # Always use Ninja on Windows (it's required for this project)
    CMAKE_GENERATOR := Ninja
    TEMP_DIR := $(if $(TEMP),$(TEMP),/tmp)
    MKDIR := cmake -E make_directory
    # Native Windows has no Unix-style timeout command
    TIMEOUT_CMD :=
else
    UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)
    ifneq ($(filter MINGW% MSYS% CYGWIN%,$(UNAME_S)),)
        PLATFORM := windows
        CMAKE_PRESET := windows-clang-release
        CMAKE_DEBUG_PRESET := windows-clang-debug
        EXE_EXT := .exe
        PYTHON := python
        # MSYS/MinGW uses Unix commands
        RM := rm -f
        RMDIR := rm -rf $(BUILD_DIR)
        RMDIR_BIN := rm -rf $(BIN_DIR)/lib
        MKDIR := mkdir -p
        TIMEOUT_CMD := timeout
        NULL_DEV := /dev/null
        NINJA_EXISTS := $(shell command -v ninja >/dev/null 2>&1 && echo yes || echo no)
        CMAKE_GENERATOR := $(if $(filter yes,$(NINJA_EXISTS)),Ninja,Unix Makefiles)
        TEMP_DIR := /tmp
    else ifeq ($(UNAME_S),Darwin)
        PLATFORM := darwin
        CMAKE_PRESET := macos-clang-release
        CMAKE_DEBUG_PRESET := macos-clang-debug
        EXE_EXT :=
        PYTHON := python3
        # Unix commands
        RM := rm -f
        RMDIR := rm -rf $(BUILD_DIR)
        RMDIR_BIN := rm -rf $(BIN_DIR)/lib
        MKDIR := mkdir -p
        # macOS doesn't ship GNU timeout; use it only if available
        TIMEOUT_CMD := $(shell command -v timeout >/dev/null 2>&1 && echo timeout || echo)
        NULL_DEV := /dev/null
        NINJA_EXISTS := $(shell command -v ninja >/dev/null 2>&1 && echo yes || echo no)
        CMAKE_GENERATOR := $(if $(filter yes,$(NINJA_EXISTS)),Ninja,Unix Makefiles)
        TEMP_DIR := /tmp
    else
        PLATFORM := linux
        CMAKE_PRESET := linux-gcc-release
        CMAKE_DEBUG_PRESET := linux-gcc-debug
        EXE_EXT :=
        PYTHON := python3
        # Unix commands
        RM := rm -f
        RMDIR := rm -rf $(BUILD_DIR)
        RMDIR_BIN := rm -rf $(BIN_DIR)/lib
        MKDIR := mkdir -p
        TIMEOUT_CMD := timeout
        NULL_DEV := /dev/null
        NINJA_EXISTS := $(shell command -v ninja >/dev/null 2>&1 && echo yes || echo no)
        CMAKE_GENERATOR := $(if $(filter yes,$(NINJA_EXISTS)),Ninja,Unix Makefiles)
        TEMP_DIR := /tmp
    endif
endif

#------------------------------------------------------------------------------
# Configuration
#------------------------------------------------------------------------------
BUILD_DIR := build
BIN_DIR := bin
SN := $(BIN_DIR)/sn$(EXE_EXT)

# Allow preset override
PRESET ?= $(CMAKE_PRESET)


#------------------------------------------------------------------------------
# Default target
#------------------------------------------------------------------------------
all: build

#------------------------------------------------------------------------------
# build - Configure and build the compiler
#------------------------------------------------------------------------------
# Select compiler based on platform
CMAKE_C_COMPILER := $(if $(filter windows,$(PLATFORM)),clang,$(if $(filter darwin,$(PLATFORM)),clang,gcc))

build:
	@echo "Building Sindarin compiler..."
	@echo "Platform: $(PLATFORM)"
	@echo "Generator: $(CMAKE_GENERATOR)"
	cmake -S . -B $(BUILD_DIR) -G "$(CMAKE_GENERATOR)" \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_COMPILER=$(CMAKE_C_COMPILER)
	cmake --build $(BUILD_DIR)
	@echo ""
	@echo "Build complete!"
	@echo "Compiler: $(SN)"

#------------------------------------------------------------------------------
# rebuild - Clean and build
#------------------------------------------------------------------------------
rebuild: clean build

#------------------------------------------------------------------------------
# configure - Just configure CMake (useful for IDE integration)
#------------------------------------------------------------------------------
configure:
	@echo "Configuring with preset: $(PRESET)"
	cmake --preset $(PRESET)

#------------------------------------------------------------------------------
# clean - Remove build artifacts (using cmake -E for cross-platform compatibility)
#------------------------------------------------------------------------------
clean:
	@echo "Cleaning build artifacts..."
	-cmake -E rm -rf $(BUILD_DIR)
	-cmake -E rm -f $(BIN_DIR)/sn$(EXE_EXT) $(BIN_DIR)/tests$(EXE_EXT)
	-cmake -E rm -rf $(BIN_DIR)/lib
	-cmake -E rm -rf $(BIN_DIR)/deps
	@echo "Cleaning test temp directories..."
	$(PYTHON) -c "import glob, shutil, tempfile, os; [shutil.rmtree(d, ignore_errors=True) for d in glob.glob(os.path.join(tempfile.gettempdir(), 'sn_test_*'))]"
	@echo "Clean complete."

#------------------------------------------------------------------------------
# run - Compile and run samples/main.sn
#------------------------------------------------------------------------------
run: build
	@echo "Running samples/main.sn..."
	$(SN) samples/main.sn -o $(TEMP_DIR)/hello-world$(EXE_EXT) -l 3
	$(TEMP_DIR)/hello-world$(EXE_EXT)

#------------------------------------------------------------------------------
# Test targets - Delegate to Python test runner
#------------------------------------------------------------------------------
test: build test-arena
	@echo "Running all tests..."
	$(PYTHON) scripts/run_tests.py all --verbose

test-unit: build
	@$(PYTHON) scripts/run_tests.py unit --verbose

test-cgen: build
	@$(PYTHON) scripts/run_tests.py cgen --verbose

test-integration: build
	@$(PYTHON) scripts/run_tests.py integration --verbose

test-integration-errors: build
	@$(PYTHON) scripts/run_tests.py integration-errors --verbose

test-explore: build
	@$(PYTHON) scripts/run_tests.py explore --verbose

test-explore-errors: build
	@$(PYTHON) scripts/run_tests.py explore-errors --verbose

#------------------------------------------------------------------------------
# arena - Build the managed arena library (standalone)
#------------------------------------------------------------------------------
ARENA_DIR := src/runtime/arena
ARENA_BUILD := $(BUILD_DIR)/arena
ARENA_SRCS := $(ARENA_DIR)/managed_arena.c $(ARENA_DIR)/managed_arena_gc.c
ARENA_TEST_SRCS := $(ARENA_DIR)/tests/test_main.c \
	$(ARENA_DIR)/tests/test_alloc.c \
	$(ARENA_DIR)/tests/test_pin.c \
	$(ARENA_DIR)/tests/test_reassignment.c \
	$(ARENA_DIR)/tests/test_gc.c \
	$(ARENA_DIR)/tests/test_concurrency.c \
	$(ARENA_DIR)/tests/test_hierarchy.c \
	$(ARENA_DIR)/tests/test_api.c \
	$(ARENA_DIR)/tests/test_stress_profiles.c \
	$(ARENA_DIR)/tests/test_race_detection.c \
	$(ARENA_DIR)/tests/test_race_detection_scaling.c \
	$(ARENA_DIR)/tests/test_race_detection_mixed_lifecycle.c \
	$(ARENA_DIR)/tests/test_race_detection_compaction.c \
	$(ARENA_DIR)/tests/test_race_detection_table_stability.c \
	$(ARENA_DIR)/tests/test_race_detection_pins.c \
	$(ARENA_DIR)/tests/test_race_detection_hierarchy.c \
	$(ARENA_DIR)/tests/test_race_detection_destroy_reset.c \
	$(ARENA_DIR)/tests/test_race_detection_recycling_clone.c
ARENA_TEST_BIN := $(BIN_DIR)/test_arena$(EXE_EXT)
ARENA_CFLAGS := -Wall -Wextra -g -pthread
# ASAN for arena tests (override with ARENA_SANITIZE= to disable, e.g. on LLVM-MinGW)
ARENA_SANITIZE ?= -fsanitize=address

arena:
	@echo "Building managed arena..."
	@$(MKDIR) $(ARENA_BUILD)
	$(CMAKE_C_COMPILER) $(ARENA_CFLAGS) $(ARENA_SANITIZE) \
		-c $(ARENA_DIR)/managed_arena.c -o $(ARENA_BUILD)/managed_arena.o
	$(CMAKE_C_COMPILER) $(ARENA_CFLAGS) $(ARENA_SANITIZE) \
		-c $(ARENA_DIR)/managed_arena_gc.c -o $(ARENA_BUILD)/managed_arena_gc.o
	@echo "Managed arena built."

#------------------------------------------------------------------------------
# test-arena - Build and run managed arena tests
#------------------------------------------------------------------------------
test-arena:
	@echo "Building and running managed arena tests..."
	@$(MKDIR) $(ARENA_BUILD)
	@$(MKDIR) $(BIN_DIR)
	$(CMAKE_C_COMPILER) $(ARENA_CFLAGS) $(ARENA_SANITIZE) \
		$(ARENA_SRCS) $(ARENA_TEST_SRCS) \
		-o $(ARENA_TEST_BIN)
	@echo ""
	$(if $(TIMEOUT_CMD),$(TIMEOUT_CMD) 30) $(ARENA_TEST_BIN)

#------------------------------------------------------------------------------
# install - Install to ~/.sn/ (global user installation)
#------------------------------------------------------------------------------
SN_HOME := $(HOME)/.sn
SN_LIB_DIR := $(SN_HOME)/lib/sindarin
SN_BIN_DIR := $(SN_HOME)/bin

install: build
	@echo "Installing Sindarin compiler to $(SN_HOME)..."
	@$(MKDIR) $(SN_LIB_DIR)
	@$(MKDIR) $(SN_BIN_DIR)
	@echo "  Copying compiler binary..."
	@cp $(BIN_DIR)/sn$(EXE_EXT) $(SN_LIB_DIR)/sn$(EXE_EXT)
	@echo "  Copying configuration..."
	@cp $(BIN_DIR)/sn.cfg $(SN_LIB_DIR)/sn.cfg
	@echo "  Copying runtime headers..."
	@rm -rf $(SN_LIB_DIR)/include
	@cp -r $(BIN_DIR)/include $(SN_LIB_DIR)/include
	@echo "  Copying runtime library..."
	@rm -rf $(SN_LIB_DIR)/lib
	@cp -r $(BIN_DIR)/lib $(SN_LIB_DIR)/lib
ifeq ($(PLATFORM),windows)
	@echo "  Copying binary to bin directory..."
	@cp $(BIN_DIR)/sn$(EXE_EXT) $(SN_BIN_DIR)/sn$(EXE_EXT)
	@echo ""
	@echo "Installation complete!"
	@echo "  Binary: $(SN_LIB_DIR)/sn$(EXE_EXT)"
	@echo "  Executable: $(SN_BIN_DIR)/sn$(EXE_EXT)"
else
	@echo "  Creating symlink..."
	@rm -f $(SN_BIN_DIR)/sn$(EXE_EXT)
	@ln -s ../lib/sindarin/sn$(EXE_EXT) $(SN_BIN_DIR)/sn$(EXE_EXT)
	@echo ""
	@echo "Installation complete!"
	@echo "  Binary: $(SN_LIB_DIR)/sn$(EXE_EXT)"
	@echo "  Symlink: $(SN_BIN_DIR)/sn$(EXE_EXT) -> ../lib/sindarin/sn$(EXE_EXT)"
endif

#------------------------------------------------------------------------------
# package - Create distributable packages
#------------------------------------------------------------------------------
package: build
	@echo "Creating packages..."
	@cd $(BUILD_DIR) && cpack

#------------------------------------------------------------------------------
# setup - Download pre-built dependencies using platform-specific installer
#------------------------------------------------------------------------------
ifeq ($(PLATFORM),windows)
setup:
	@echo "Setting up build dependencies for Windows..."
	@powershell -NoProfile -ExecutionPolicy Bypass -Command "New-Item -ItemType Directory -Force -Path libs | Out-Null; cd libs; irm https://raw.githubusercontent.com/SindarinSDK/sindarin-pkg-libs-v2/main/scripts/install.ps1 | iex"
	@echo "Pre-built libraries ready!"
	@echo "Run 'make build' to build the compiler."
else
setup:
	@echo "Setting up build dependencies for $(PLATFORM)..."
	@mkdir -p libs && cd libs && curl -fsSL https://raw.githubusercontent.com/SindarinSDK/sindarin-pkg-libs-v2/main/scripts/install.sh | bash
	@echo "Pre-built libraries ready!"
	@echo "Run 'make build' to build the compiler."
endif

#------------------------------------------------------------------------------
# libs - Alias for setup (backwards compatibility)
#------------------------------------------------------------------------------
libs: setup

#------------------------------------------------------------------------------
# help - Show available targets
#------------------------------------------------------------------------------
help:
	@echo "Sindarin Compiler - Build System"
	@echo ""
	@echo "Quick Start:"
	@echo "  make build        Build the compiler"
	@echo "  make test         Run all tests"
	@echo "  make run          Compile and run samples/main.sn"
	@echo ""
	@echo "Build Targets:"
	@echo "  make build        Build compiler (auto-detects platform)"
	@echo "  make rebuild      Clean and build"
	@echo "  make configure    Configure CMake only"
	@echo "  make clean        Remove build artifacts"
	@echo ""
	@echo "Test Targets:"
	@echo "  make test                   Run all tests"
	@echo "  make test-unit              Run unit tests only"
	@echo "  make test-cgen              Run code generation tests (compare generated C)"
	@echo "  make test-integration       Run integration tests"
	@echo "  make test-integration-errors Run integration error tests"
	@echo "  make test-explore           Run exploratory tests"
	@echo "  make test-explore-errors    Run exploratory error tests"
	@echo "  make arena                  Build managed arena library"
	@echo "  make test-arena             Build and run managed arena tests"
	@echo ""
	@echo "Distribution Targets:"
	@echo "  make install      Install to ~/.sn/ (overwrites global compiler)"
	@echo "  make package      Create distributable packages"
	@echo ""
	@echo "Setup:"
	@echo "  make setup        Download pre-built dependencies"
	@echo "  make libs         Alias for 'make setup'"
	@echo ""
	@echo "CMake Presets (Advanced):"
	@echo "  cmake --preset linux-gcc-release    Linux with GCC"
	@echo "  cmake --preset linux-clang-release  Linux with Clang"
	@echo "  cmake --preset windows-clang-release Windows with Clang"
	@echo "  cmake --preset macos-clang-release  macOS with Clang"
	@echo ""
	@echo "  Then: cmake --build --preset <preset-name>"
	@echo ""
	@echo "Environment Variables:"
	@echo "  PRESET=<name>     Override CMake preset"
	@echo "  SN_CC=<compiler>  C compiler for generated code"
	@echo "  SN_CFLAGS=<flags> Extra compiler flags"
	@echo "  SN_LDFLAGS=<flags> Extra linker flags"
	@echo ""
	@echo "Platform: $(PLATFORM)"
	@echo "Preset: $(PRESET)"
