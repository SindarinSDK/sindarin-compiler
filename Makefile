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
.PHONY: test-unit test-cgen test-rgen test-rgen-byte-strings test-mgen test-integration test-integration-errors
.PHONY: test-explore test-explore-errors test-rust-closures test-rust-concurrency test-rust-native-tagged test-rust-native-extra test-rust-native-origin test-rust-native-errors test-rust-toolchain
.PHONY: configure install package setup hooks

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
    # Use cmake -E for cross-platform file operations on native Windows
    MKDIR := cmake -E make_directory
    CP := cmake -E copy
    CP_DIR := cmake -E copy_directory
    RM := cmake -E rm -f
    RM_DIR := cmake -E rm -rf
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
        CP := cp
        CP_DIR := cp -r
        RM_DIR := rm -rf
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
        CP := cp
        CP_DIR := cp -r
        RM_DIR := rm -rf
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
        CP := cp
        CP_DIR := cp -r
        RM_DIR := rm -rf
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

build: hooks
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
	-cmake -E rm -f $(BIN_DIR)/sn$(EXE_EXT) $(BIN_DIR)/tests$(EXE_EXT) $(BIN_DIR)/sn_fake_rustc$(EXE_EXT)
	-cmake -E rm -rf $(BIN_DIR)/lib
	-cmake -E rm -rf $(BIN_DIR)/deps
	@echo "Cleaning test temp directories..."
	$(PYTHON) -c "import glob, shutil, tempfile, os; [shutil.rmtree(d, ignore_errors=True) for d in glob.glob(os.path.join(tempfile.gettempdir(), 'sn_test_*')) + glob.glob(os.path.join(tempfile.gettempdir(), 'sn_rustc_*'))]"
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

test-cgen: build
	@$(PYTHON) scripts/run_tests.py cgen --verbose

test-rgen: build
	@$(PYTHON) scripts/run_rust_tests.py rgen --verbose
	@$(PYTHON) tests/rgen/byte_string_compare.py

test-rgen-byte-strings: build
	@$(PYTHON) tests/rgen/byte_string_compare.py

test-mgen: build
	@$(PYTHON) scripts/run_tests.py mgen --verbose

test-integration: build
	@$(PYTHON) scripts/run_tests.py integration --verbose

test-integration-errors: build
	@$(PYTHON) scripts/run_tests.py integration-errors --verbose

test-explore: build
	@$(PYTHON) scripts/run_tests.py explore --verbose

test-explore-errors: build
	@$(PYTHON) scripts/run_tests.py explore-errors --verbose

test-rust-native-tagged: build
	@$(PYTHON) scripts/run_rust_tests.py rust-native-tagged --verbose

test-rust-native-extra: build
	@$(PYTHON) scripts/run_rust_tests.py rust-native-extra --verbose

test-rust-native-origin: build
	@$(PYTHON) scripts/run_rust_tests.py rust-native-origin --verbose

test-rust-native-errors: build
	@$(PYTHON) scripts/run_rust_tests.py rust-native-errors --verbose

test-rust-closures: build
	@$(PYTHON) scripts/run_rust_tests.py rust-closure-values --verbose
	@$(PYTHON) scripts/run_rust_tests.py rust-closure-values-errors --verbose

test-rust-concurrency: build
	@$(PYTHON) scripts/run_rust_tests.py rust-concurrency --verbose
	@$(PYTHON) scripts/run_rust_tests.py rust-concurrency-promoted --verbose
	@$(PYTHON) scripts/run_rust_tests.py rust-concurrency-errors --verbose

test-rust-toolchain: build
	@$(PYTHON) scripts/run_rust_tests.py rust-toolchain --verbose

# Positive, source-identical C/Rust checks at every source optimization level.
# Keep this separate from historical Rust-only admission/snapshot fixtures.
.PHONY: test-rust-parity-core
test-rust-parity-core: build
	@$(PYTHON) scripts/check_rust_parity.py --compiler $(SN) --output .sn/rust-parity-core.json \
		tests/rgen/by_value_scalar_parameter_assignment.sn \
		tests/rgen/by_value_scalar_parameter_assignment_unchecked.sn \
		tests/rgen/checked_numeric_mutations.sn \
		tests/rgen/iterator_protocol_numeric_mutations.sn \
		tests/rgen/resolved_callable_methods.sn \
		tests/rgen/receiver_array_alias.sn \
		tests/rgen/receiver_array_alias_forward_recursive.sn \
		tests/rgen/receiver_array_alias_identity.sn \
		tests/rgen/receiver_array_alias_minimal.sn \
		tests/rgen/receiver_array_alias_multiple.sn

# GCC floating atomic postfix needs libatomic on the Linux toolchain. This is
# an explicit, recorded gate-only override; normal compiler configuration stays unchanged.
ifeq ($(PLATFORM),linux)
RUST_CONCURRENCY_C_LINK_ARGS ?= --c-ldlibs "-lpthread -lm -latomic"
endif
.PHONY: test-rust-parity-concurrency
test-rust-parity-concurrency: build
	@$(PYTHON) scripts/check_rust_parity.py --compiler $(SN) --require-count 17 \
		$(RUST_CONCURRENCY_C_LINK_ARGS) --output .sn/rust-parity-concurrency.json \
		tests/rust-concurrency/*.sn tests/rgen/concurrency-promoted/*.sn

.PHONY: test-rust-parity-thread-ownership
test-rust-parity-thread-ownership: build
	@$(PYTHON) scripts/check_rust_parity.py --compiler $(SN) --require-count 23 \
		--arithmetic-mode default --arithmetic-mode checked --arithmetic-mode unchecked \
		--output .sn/rust-parity-thread-ownership.json \
		tests/rust-thread-ownership/*.sn tests/rust-thread-array-identity/*.sn*

.PHONY: test-rust-parity-thread-receivers
test-rust-parity-thread-receivers: build
	@$(PYTHON) scripts/check_rust_parity.py --compiler $(SN) --require-count 10 \
		--arithmetic-mode default --arithmetic-mode checked --arithmetic-mode unchecked \
		--output .sn/rust-parity-thread-receivers.json \
		tests/rust-thread-receivers/*.sn \
		tests/integration/test_thread_spawn_self_method.sn \
		tests/integration/test_thread_struct_param.sn \
		tests/integration/test_pass_self_to_function.sn

#------------------------------------------------------------------------------
# install - Install to ~/.sn/ (global user installation)
#------------------------------------------------------------------------------
ifeq ($(PLATFORM),windows)
    SN_HOME := $(USERPROFILE)/.sn
else
    SN_HOME := $(HOME)/.sn
endif
SN_LIB_DIR := $(SN_HOME)/lib/sindarin
SN_BIN_DIR := $(SN_HOME)/bin

install: build
	@echo "Installing Sindarin compiler to $(SN_HOME)..."
	@$(MKDIR) $(SN_LIB_DIR)
	@$(MKDIR) $(SN_BIN_DIR)
	@echo "  Copying compiler binary..."
	@$(CP) $(BIN_DIR)/sn$(EXE_EXT) $(SN_LIB_DIR)/sn$(EXE_EXT)
	@echo "  Copying configuration..."
	@$(CP) $(BIN_DIR)/sn.linux.cfg $(SN_LIB_DIR)/sn.linux.cfg
	@$(CP) $(BIN_DIR)/sn.darwin.cfg $(SN_LIB_DIR)/sn.darwin.cfg
	@$(CP) $(BIN_DIR)/sn.windows.cfg $(SN_LIB_DIR)/sn.windows.cfg
	@echo "  Copying runtime headers..."
	@$(RM_DIR) $(SN_LIB_DIR)/include
	@$(CP_DIR) $(BIN_DIR)/include $(SN_LIB_DIR)/include
	@echo "  Copying runtime library..."
	@$(RM_DIR) $(SN_LIB_DIR)/lib
	@$(CP_DIR) $(BIN_DIR)/lib $(SN_LIB_DIR)/lib
	@echo "  Copying templates..."
	@$(RM_DIR) $(SN_LIB_DIR)/templates
	@$(CP_DIR) $(BIN_DIR)/templates $(SN_LIB_DIR)/templates
ifeq ($(PLATFORM),windows)
	@echo "  Copying binary to bin directory..."
	@$(CP) $(BIN_DIR)/sn$(EXE_EXT) $(SN_BIN_DIR)/sn$(EXE_EXT)
	@echo ""
	@echo "Installation complete!"
	@echo "  Binary: $(SN_LIB_DIR)/sn$(EXE_EXT)"
	@echo "  Executable: $(SN_BIN_DIR)/sn$(EXE_EXT)"
else
	@echo "  Creating symlink..."
	@$(RM) $(SN_BIN_DIR)/sn$(EXE_EXT)
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
# setup - Bootstrap compiler and install pre-built dependencies
#------------------------------------------------------------------------------
ifeq ($(PLATFORM),windows)
setup:
	@echo "Setting up build dependencies for Windows..."
	@powershell -NoProfile -ExecutionPolicy Bypass -File scripts/install.ps1
	@export PATH="$$HOME/.sn/bin:$$PATH" && sn --install
	@echo "Pre-built libraries ready!"
	@echo "Run 'make build' to build the compiler."
else
setup:
	@echo "Setting up build dependencies for $(PLATFORM)..."
	@bash scripts/install.sh
	@export PATH="$$HOME/.sn/bin:$$PATH" && sn --install
	@echo "Pre-built libraries ready!"
	@echo "Run 'make build' to build the compiler."
endif

#------------------------------------------------------------------------------
# hooks - Configure git to use tracked pre-commit hooks
#------------------------------------------------------------------------------
ifeq ($(PLATFORM),windows)
hooks:
	-@git config core.hooksPath .githooks >NUL 2>&1
else
hooks:
	-@git config core.hooksPath .githooks 2>/dev/null
endif

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
	@echo "  make test-rgen              Run Rust generation tests"
	@echo "  make test-rgen-byte-strings Run exact raw-byte C/Rust string comparisons"
	@echo "  make test-rust-native-tagged Run C/Rust parity on unchanged tag-79c20b fixtures"
	@echo "  make test-rust-native-extra Run post-tag Rust-native extra coverage"
	@echo "  make test-rust-native-origin Run imported native origin tests"
	@echo "  make test-rust-native-errors Run Rust native scalar error tests"
	@echo "  make test-mgen              Run model generation tests (compare JSON model)"
	@echo "  make test-integration       Run integration tests"
	@echo "  make test-integration-errors Run integration error tests"
	@echo "  make test-explore           Run exploratory tests"
	@echo "  make test-explore-errors    Run exploratory error tests"
	@echo "  make test-rust-closures     Run promoted Rust closure-value tests"
	@echo "  make test-rust-concurrency  Run Rust concurrency and promoted concurrency tests"
	@echo "  make test-rust-toolchain    Run Rust toolchain and shared artifact lifecycle tests"
	@echo ""
	@echo "Distribution Targets:"
	@echo "  make install      Install to ~/.sn/ (overwrites global compiler)"
	@echo "  make package      Create distributable packages"
	@echo ""
	@echo "Setup:"
	@echo "  make setup        Download pre-built dependencies"
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

.PHONY: test-rust-parity-reflection
test-rust-parity-reflection: build
	@$(PYTHON) scripts/check_rust_parity.py \
		tests/rgen/typeof_sized_array.sn \
		tests/rgen/typeof_sized_array_contract.sn \
		tests/rgen/typeof_metadata.sn \
		tests/integration/test_typeof.sn \
		--require-count 4 --arithmetic-mode default --arithmetic-mode checked \
		--arithmetic-mode unchecked --output .sn/rust-parity-reflection.json

.PHONY: test-rust-parity-sized-defaults
test-rust-parity-sized-defaults: build
	@$(PYTHON) scripts/check_rust_parity.py \
		tests/integration/test_sized_array_syntax.sn \
		tests/rgen/sized_array_defaults.sn \
		tests/rgen/typeof_sized_array.sn \
		--require-count 3 --arithmetic-mode default --arithmetic-mode checked \
		--arithmetic-mode unchecked --output .sn/rust-parity-sized-defaults.json

.PHONY: test-rust-parity-native-managed
test-rust-parity-native-managed: build
	@$(PYTHON) scripts/check_rust_parity.py \
		tests/integration/test_pointer_unwrap.sn \
		tests/integration/test_interop_pointers.sn \
		tests/rust-native/scalar_managed_pointer_bridge.sn \
		tests/rust-native/native_as_ref_char_same_place.sn \
		tests/rust-native/native_as_ref_char_temp_collision.sn \
		tests/rust-native/native_as_ref_same_place.sn \
		tests/rust-native/native_string_escape.sn \
		--require-count 7 --arithmetic-mode default --arithmetic-mode checked \
		--arithmetic-mode unchecked --output .sn/rust-parity-native-managed.json

.PHONY: test-rust-parity-stdio-order
test-rust-parity-stdio-order: build
	@$(PYTHON) tests/rgen/stdio_order_compare.py

.PHONY: test-rust-parity-mixed-integral
test-rust-parity-mixed-integral: build
	@$(PYTHON) scripts/check_rust_parity.py \
		tests/exploratory/test_byte_array.sn \
		tests/exploratory/test_byte_primitives.sn \
		tests/exploratory/test_gcc_edge_arrays.sn \
		tests/exploratory/test_mixed_numeric_types.sn \
		tests/integration/test_scope_byte_var.sn \
		tests/rgen/mixed_integral_binary_boundaries.sn \
		tests/integration/test_int32_arithmetic.sn \
		tests/rgen/tagged_byte_promotions_checked.sn \
		tests/integration/test_uint32_arithmetic.sn \
		tests/rgen/tagged_uint32_literal_observation_contexts.sn \
		--require-count 10 --arithmetic-mode default --arithmetic-mode checked \
		--arithmetic-mode unchecked --output .sn/rust-parity-mixed-integral.json
	@$(PYTHON) tests/rgen/mixed_integral_diagnostics_compare.py

.PHONY: test-rust-parity-float-conversions
test-rust-parity-float-conversions: build
	@$(PYTHON) scripts/check_rust_parity.py \
		tests/integration/test_interop_types.sn \
		tests/rgen/float_conversion_boundaries.sn \
		tests/rgen/float_callable_conversions.sn \
		tests/rgen/float_mutation_precision.sn \
		tests/rgen/by_value_parameter_direct_assignment_mixed_float_double.sn \
		tests/rgen/floating_as_ref_parameter_mixed_type_mutation.sn \
		tests/rgen/floating_compound_mixed_float_double.sn \
		tests/rgen/iterator_protocol_mixed_float_double_mutation.sn \
		tests/rgen/closure_values_mixed_argument.sn \
		tests/rust-native/scalar_float_conversions.sn \
		tests/integration/test_float_arithmetic.sn \
		tests/integration/test_op_double_arith.sn \
		tests/rust-float-sync/float_cell_conversions.sn \
		$(RUST_CONCURRENCY_C_LINK_ARGS) \
		--require-count 13 --arithmetic-mode default --arithmetic-mode checked \
		--arithmetic-mode unchecked --output .sn/rust-parity-float-conversions.json

.PHONY: test-rust-parity-floating-arrays
test-rust-parity-floating-arrays: build
	@$(PYTHON) scripts/check_rust_parity.py \
		tests/rgen/float_array_search.sn \
		tests/rgen/double_array_search.sn \
		tests/rgen/float_array_search_double_needle.sn \
		tests/rgen/floating_array_search_representation.sn \
		tests/rgen/floating_array_search_call_boundaries.sn \
		tests/rgen/floating_array_storage_bytes.sn \
		tests/rust-float-search/floating_search_expression_widths.sn \
		tests/rust-float-search/floating_array_capture_snapshots.sn \
		tests/rust-native/scalar_float_array_representation.sn \
		--require-count 9 --arithmetic-mode default --arithmetic-mode checked \
		--arithmetic-mode unchecked --output .sn/rust-parity-floating-arrays.json

.PHONY: test-rust-parity-floating-equality
test-rust-parity-floating-equality: build
	@$(PYTHON) scripts/check_rust_parity.py \
		tests/rgen/floating_array_equality_representation.sn \
		tests/rgen/floating_array_equality_mixed_width.sn \
		tests/rgen/floating_array_equality_contexts.sn \
		tests/rgen/floating_array_equality_mutation.sn \
		--require-count 4 --arithmetic-mode default --arithmetic-mode checked \
		--arithmetic-mode unchecked --output .sn/rust-parity-floating-equality.json

.PHONY: test-rust-parity-numeric-places
test-rust-parity-numeric-places: build
	@$(PYTHON) scripts/check_rust_parity.py \
		tests/integration/test_compound_assignment.sn \
		tests/rgen/floating_compound_nested_field.sn \
		tests/rgen/floating_compound_unstable_place.sn \
		tests/rgen/floating_nested_as_ref_parameter_postfix.sn \
		tests/rgen/floating_postfix_array_index.sn \
		tests/rgen/floating_postfix_nested_field.sn \
		tests/rgen/nonfloating_nested_by_value_parameter_postfix.sn \
		tests/rgen/numeric_places_locked_struct.sn \
		tests/rgen/numeric_places_array_captures.sn \
		tests/rgen/numeric_places_float_widths.sn \
		tests/rgen/numeric_places_integer_widths.sn \
		tests/rgen/numeric_places_nested_counts.sn \
		tests/rgen/numeric_places_parameters.sn \
		tests/rgen/numeric_places_rhs_effects.sn \
		tests/rgen/numeric_places_rhs_expression_widths.sn \
		tests/rgen/numeric_places_temporary_owners.sn \
		tests/rust-numeric-places/numeric_place_thread_parameters.sn \
		--require-count 17 --arithmetic-mode default --arithmetic-mode checked \
		--arithmetic-mode unchecked --output .sn/rust-parity-numeric-places.json

.PHONY: test-rust-parity-capture-index
test-rust-parity-capture-index: build
	@$(PYTHON) scripts/check_rust_parity.py \
		tests/rgen/capture_array_index_nested.sn \
		tests/rgen/capture_array_index_result.sn \
		tests/rgen/capture_array_index_shared.sn \
		tests/rgen/capture_array_index_state.sn \
		tests/rgen/capture_array_index_widths.sn \
		--require-count 5 --arithmetic-mode default --arithmetic-mode checked \
		--arithmetic-mode unchecked --output .sn/rust-parity-capture-index.json

.PHONY: test-rust-parity-sync-character
test-rust-parity-sync-character: build
	@$(PYTHON) scripts/check_rust_parity.py \
		tests/integration/test_sync_byte_char.sn \
		tests/exploratory/test_sync_byte_threading.sn \
		tests/rgen/sync_character_global_postfix.sn \
		tests/rgen/sync_character_local_wrap.sn \
		tests/rgen/sync_character_thread_wrap.sn \
		--require-count 5 --arithmetic-mode default --arithmetic-mode checked \
		--arithmetic-mode unchecked --output .sn/rust-parity-sync-character.json

.PHONY: test-rust-parity-ctype
test-rust-parity-ctype: build
	@$(PYTHON) scripts/check_rust_parity.py \
		tests/integration/test_char_methods.sn \
		tests/exploratory/test_str_comprehensive.sn \
		tests/rgen/character_methods_basic.sn \
		tests/rgen/character_methods_receivers.sn \
		tests/rgen/character_methods_predicates.sn \
		tests/rgen/string_ctype_bytes.sn \
		tests/rgen/string_operations.sn \
		tests/rust-native/character_ctype_locale.sn \
		--require-count 8 --arithmetic-mode default --arithmetic-mode checked \
		--arithmetic-mode unchecked --output .sn/rust-parity-ctype.json
	@$(PYTHON) scripts/check_rust_ctype_oracles.py .sn/rust-parity-ctype.json

.PHONY: test-rust-parity-byte-encoding
test-rust-parity-byte-encoding: build
	@$(PYTHON) scripts/check_rust_parity.py \
		tests/integration/test_byte_encoding.sn \
		tests/rgen/byte_encoding_domain.sn \
		tests/rgen/byte_encoding_latin1_domain.sn \
		tests/rgen/byte_encoding_latin1_nul.sn \
		tests/rgen/byte_encoding_receivers.sn \
		tests/rgen/byte_encoding_parameters.sn \
		tests/rgen/byte_encoding_sync_capture_state.sn \
		tests/rgen/array_join_hygiene.sn \
		tests/rust-native/byte_encoding_object_view.sn \
		--require-count 9 --arithmetic-mode default --arithmetic-mode checked \
		--arithmetic-mode unchecked --output .sn/rust-parity-byte-encoding.json
	@$(PYTHON) scripts/check_rust_byte_encoding_oracles.py .sn/rust-parity-byte-encoding.json

.PHONY: test-rust-parity-exit
test-rust-parity-exit: build
	@$(PYTHON) tests/rgen/builtin_exit_compare.py

.PHONY: test-rust-parity-main-return
test-rust-parity-main-return: build
	@$(PYTHON) tests/rgen/main_return_compare.py

.PHONY: test-rust-parity-match-control
test-rust-parity-match-control: build
	@$(PYTHON) scripts/check_rust_parity.py \
		tests/integration/test_match_return_arms.sn \
		tests/exploratory/test_match_primitives.sn \
		tests/exploratory/test_match_native.sn \
		tests/exploratory/test_match_memory.sn \
		tests/rgen/value_match_char_result.sn \
		tests/rgen/value_match_multiline_body.sn \
		tests/rgen/value_match_bool_multiline_body.sn \
		tests/rgen/value_match_string_multiline_body.sn \
		tests/rgen/value_match_float_multiline_body.sn \
		tests/rgen/value_match_break_prefix.sn \
		tests/rgen/value_match_continue_prefix.sn \
		tests/rgen/match_control_char_bytes.sn \
		tests/rgen/match_control_char_subject.sn \
		tests/rgen/match_control_callable_returns.sn \
		tests/rgen/match_control_bool_returns.sn \
		tests/rgen/match_control_arm_scopes.sn \
		tests/rgen/match_control_local_atomic_lock.sn \
		tests/rgen/match_control_char_places.sn \
		tests/rgen/match_control_captured_char_array.sn \
		tests/rgen/match_control_probes/char_subject.sn.raw \
		--require-count 20 --arithmetic-mode default --arithmetic-mode checked \
		--arithmetic-mode unchecked --output .sn/rust-parity-match-control.json
	@$(PYTHON) scripts/check_rust_match_control_oracles.py .sn/rust-parity-match-control.json

.PHONY: test-rust-parity-sizeof
test-rust-parity-sizeof: build
	@$(PYTHON) scripts/check_rust_parity.py \
		tests/integration/test_struct_stack_alloc.sn \
		tests/integration/test_struct_heap_alloc.sn \
		tests/integration/test_struct_sizeof_equality.sn \
		tests/integration/test_sizeof.sn \
		tests/rgen/sizeof_c_unevaluated.sn \
		tests/rgen/sizeof_c_member_unevaluated.sn \
		tests/rgen/sizeof_c_hygiene.sn \
		tests/rust-native/scalar_local_aggregate.sn \
		tests/rust-native/scalar_local_aggregate_results.sn \
		tests/rust-native/scalar_operator_local_aggregate.sn \
		tests/rgen/sizeof_struct.sn \
		tests/rgen/sizeof_fixed_scalars.sn \
		tests/rgen/sizeof_managed_handles.sn \
		tests/rgen/sizeof_pointer_values.sn \
		tests/rgen/sizeof_c_padding.sn \
		tests/rgen/sizeof_c_handles.sn \
		tests/rgen/sizeof_c_packed_reference_native.sn \
		tests/rgen/sizeof_c_nested_layout.sn \
		tests/rgen/sizeof-promoted/import_pure_unsupported_native_struct.sn \
		tests/rgen/resolved_operator_source_child_precedence.sn \
		tests/integration/test_struct_as_ref_chain.sn \
		tests/integration/test_struct_codegen_validation.sn \
		tests/integration/test_struct_native_defaults.sn \
		tests/integration/test_struct_packed.sn \
		--require-count 24 --arithmetic-mode default --arithmetic-mode checked \
		--arithmetic-mode unchecked --output .sn/rust-parity-sizeof.json
	@$(PYTHON) scripts/check_rust_sizeof_oracles.py .sn/rust-parity-sizeof.json

.PHONY: test-rust-parity-call-reference
test-rust-parity-call-reference: build
	@$(PYTHON) scripts/check_rust_parity.py \
		tests/rgen/sizeof_function.sn \
		tests/rgen/sizeof_interface.sn \
		tests/rgen/sizeof_nil.sn \
		tests/rgen/sizeof_void.sn \
		tests/rgen/sizeof_c_callables.sn \
		tests/rgen/sizeof_c_nonvalues.sn \
		tests/rgen/sizeof_c_interfaces.sn \
		tests/rgen/sizeof_c_callback_fields.sn \
		tests/rgen/sizeof_c_lambda_operand.sn \
		tests/rust-native/scalar_sizeof_native_lambda.sn \
		tests/integration/test_as_ref_on_ref_struct_param.sn \
		tests/rgen/reference_call_borrowed_handles.sn \
		tests/rgen/reference_call_static_handles.sn \
		tests/rgen/reference_call_member_handles.sn \
		--require-count 14 --arithmetic-mode default --arithmetic-mode checked \
		--arithmetic-mode unchecked --output .sn/rust-parity-call-reference.json
	@$(PYTHON) scripts/check_rust_call_reference_oracles.py .sn/rust-parity-call-reference.json

.PHONY: test-rust-parity-indexed-string
test-rust-parity-indexed-string: build
	@$(PYTHON) scripts/check_rust_parity.py \
		tests/integration/test_generics_functions.sn \
		tests/rgen/indexed_string_return_generic.sn \
		tests/rgen/indexed_string_return_call_kinds.sn \
		tests/rgen/indexed_string_return_computed.sn \
		tests/rgen/indexed_string_return_order.sn \
		tests/rgen/indexed_string_initializer_hygiene.sn \
		--require-count 6 --arithmetic-mode default --arithmetic-mode checked \
		--arithmetic-mode unchecked --output .sn/rust-parity-indexed-string.json
	@$(PYTHON) scripts/check_rust_indexed_string_oracles.py .sn/rust-parity-indexed-string.json

.PHONY: test-rust-parity-native-scope
test-rust-parity-native-scope: build
	@$(PYTHON) scripts/check_rust_parity.py \
		tests/integration/test_struct_native_c_interop.sn \
		tests/rust-native/scalar_c_only_helpers.sn \
		tests/integration/test_native_struct_ref_array_pass.sn \
		tests/exploratory/test_struct_zlib_style.sn \
		--require-count 4 --arithmetic-mode default --arithmetic-mode checked \
		--arithmetic-mode unchecked --output .sn/rust-parity-native-scope.json
	@$(PYTHON) scripts/check_rust_native_scope_oracles.py .sn/rust-parity-native-scope.json

.PHONY: test-rust-parity-nil-string
test-rust-parity-nil-string: build
	@$(PYTHON) scripts/check_rust_parity.py \
		tests/integration/test_nil_string.sn \
		tests/rgen/nil_string_transitions.sn \
		tests/rgen/nil_string_print_concat.sn \
		tests/rgen/nil_string_fields.sn \
		tests/rust-native/scalar_nil_string_native.sn \
		tests/rust-native/scalar_nil_string_implicit.sn \
		tests/rust-native/scalar_nil_string_default.sn \
		tests/rust-native/scalar_nil_string_calls.sn \
		--require-count 8 --arithmetic-mode default --arithmetic-mode checked \
		--arithmetic-mode unchecked --output .sn/rust-parity-nil-string.json
	@$(PYTHON) scripts/check_rust_nil_string_oracles.py .sn/rust-parity-nil-string.json
