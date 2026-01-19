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
.PHONY: test-unit test-integration test-integration-errors
.PHONY: test-explore test-explore-errors test-sdk
.PHONY: configure install package
.PHONY: setup-deps

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
test: build
	@echo "Running all tests..."
	$(PYTHON) scripts/run_tests.py all --verbose

test-unit: build
	@$(PYTHON) scripts/run_tests.py unit --verbose

test-integration: build
	@$(PYTHON) scripts/run_tests.py integration --verbose

test-integration-errors: build
	@$(PYTHON) scripts/run_tests.py integration-errors --verbose

test-explore: build
	@$(PYTHON) scripts/run_tests.py explore --verbose

test-explore-errors: build
	@$(PYTHON) scripts/run_tests.py explore-errors --verbose

test-sdk: build
	@$(PYTHON) scripts/run_tests.py sdk --verbose

#------------------------------------------------------------------------------
# install - Install to system
#------------------------------------------------------------------------------
install: build
	@echo "Installing Sindarin compiler..."
	@cmake --install $(BUILD_DIR)

#------------------------------------------------------------------------------
# package - Create distributable packages
#------------------------------------------------------------------------------
package: build
	@echo "Creating packages..."
	@cd $(BUILD_DIR) && cpack

#------------------------------------------------------------------------------
# setup-deps - Install build dependencies
#------------------------------------------------------------------------------
setup-deps:
	@echo "Setting up build dependencies..."
	@$(PYTHON) scripts/setup_deps.py

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
	@echo "  make test-integration       Run integration tests"
	@echo "  make test-integration-errors Run integration error tests"
	@echo "  make test-explore           Run exploratory tests"
	@echo "  make test-explore-errors    Run exploratory error tests"
	@echo "  make test-sdk               Run SDK tests"
	@echo ""
	@echo "Distribution Targets:"
	@echo "  make install      Install to system"
	@echo "  make package      Create distributable packages"
	@echo ""
	@echo "Setup:"
	@echo "  make setup-deps   Install build dependencies"
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
