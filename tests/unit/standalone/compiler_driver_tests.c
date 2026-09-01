// tests/unit/standalone/compiler_driver_tests.c
// Unit tests for the compiler driver (argument parsing and options)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../test_harness.h"
#include "../compiler.h"
#include "../debug.h"

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/* Create a mock argc/argv for testing */
static void make_args(int *argc, char ***argv, const char *args[], int count)
{
    *argc = count;
    *argv = (char **)args;
}

/* ============================================================================
 * Optimization Level Tests
 * ============================================================================ */

static void test_opt_level_default(void)
{
    /* Default optimization level should be OPT_LEVEL_FULL (2) */
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 2);

    arena_init(&options.arena, 1024);
    options.optimization_level = OPT_LEVEL_FULL;  /* Set default like compiler_init does */

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.optimization_level == OPT_LEVEL_FULL);

    arena_free(&options.arena);
}

static void test_opt_level_O0(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "-O0"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 3);

    arena_init(&options.arena, 1024);
    options.optimization_level = OPT_LEVEL_FULL;

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.optimization_level == OPT_LEVEL_NONE);

    arena_free(&options.arena);
}

static void test_opt_level_O1(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "-O1"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 3);

    arena_init(&options.arena, 1024);
    options.optimization_level = OPT_LEVEL_FULL;

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.optimization_level == OPT_LEVEL_BASIC);

    arena_free(&options.arena);
}

static void test_opt_level_O2(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "-O2"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 3);

    arena_init(&options.arena, 1024);
    options.optimization_level = OPT_LEVEL_NONE;
    options.arithmetic_mode = ARITH_CHECKED;

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.optimization_level == OPT_LEVEL_FULL);

    arena_free(&options.arena);
}

/* ============================================================================
 * Arithmetic Mode Tests
 * ============================================================================ */

static void test_arith_default_checked(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 2);

    arena_init(&options.arena, 1024);
    options.arithmetic_mode = ARITH_CHECKED;  /* Default */

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.arithmetic_mode == ARITH_CHECKED);

    arena_free(&options.arena);
}

static void test_arith_unchecked_flag(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "--unchecked"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 3);

    arena_init(&options.arena, 1024);
    options.arithmetic_mode = ARITH_CHECKED;

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.arithmetic_mode == ARITH_UNCHECKED);

    arena_free(&options.arena);
}

static void test_arith_checked_flag(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "--checked"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 3);

    arena_init(&options.arena, 1024);
    options.arithmetic_mode = ARITH_UNCHECKED;

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.arithmetic_mode == ARITH_CHECKED);

    arena_free(&options.arena);
}

static void test_arith_O2_explicit_defaults_unchecked(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "-O2"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 3);

    arena_init(&options.arena, 1024);
    options.arithmetic_mode = ARITH_CHECKED;
    options.optimization_level = OPT_LEVEL_NONE;

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    /* Explicit -O2 should default to unchecked arithmetic */
    assert(options.arithmetic_mode == ARITH_UNCHECKED);

    arena_free(&options.arena);
}

static void test_arith_O2_with_checked_override(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "-O2", "--checked"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 4);

    arena_init(&options.arena, 1024);
    options.arithmetic_mode = ARITH_CHECKED;
    options.optimization_level = OPT_LEVEL_NONE;

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    /* --checked should override the -O2 unchecked default */
    assert(options.arithmetic_mode == ARITH_CHECKED);

    arena_free(&options.arena);
}

/* ============================================================================
 * Output Options Tests
 * ============================================================================ */

static void test_output_file_flag(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "-o", "myprogram"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 4);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    /* Normal compilation consumes -o as the executable path. */
    assert(options.output_file == NULL);
    assert(options.executable_file != NULL);
    assert(strcmp(options.executable_file, "myprogram") == 0);

    arena_free(&options.arena);
}

static void test_emit_c_flag(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "--emit-c"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 3);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.target == TARGET_C);
    assert(options.output_kind == OUTPUT_SOURCE);

    arena_free(&options.arena);
}

static void test_keep_c_flag(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "--keep-c"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 3);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.keep_generated == 1);

    arena_free(&options.arena);
}

/* ============================================================================
 * Debug Options Tests
 * ============================================================================ */

static void test_verbose_flag(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "-v"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 3);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.verbose == 1);

    arena_free(&options.arena);
}

static void test_debug_flag(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "-g"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 3);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.debug_build == 1);

    arena_free(&options.arena);
}

static void test_log_level_flag(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "-l", "3"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 4);

    arena_init(&options.arena, 1024);
    options.log_level = DEBUG_LEVEL_ERROR;

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.log_level == DEBUG_LEVEL_INFO);

    arena_free(&options.arena);
}

static void test_log_level_verbose(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "-l", "4"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 4);

    arena_init(&options.arena, 1024);
    options.log_level = DEBUG_LEVEL_ERROR;

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.log_level == DEBUG_LEVEL_VERBOSE);

    arena_free(&options.arena);
}

/* ============================================================================
 * Update Options Tests
 * ============================================================================ */

/* ============================================================================
 * Source File Tests
 * ============================================================================ */

static void test_source_file_parsed(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "myfile.sn"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 2);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.source_file != NULL);
    assert(strcmp(options.source_file, "myfile.sn") == 0);

    arena_free(&options.arena);
}

static void test_source_file_path(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "path/to/myfile.sn"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 2);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.source_file != NULL);
    assert(strcmp(options.source_file, "path/to/myfile.sn") == 0);

    arena_free(&options.arena);
}

/* ============================================================================
 * Output Path Derivation Tests
 * ============================================================================ */

static void test_default_output_path(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "myfile.sn"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 2);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    /* Default executable should be "myfile" (without .sn) */
    assert(options.executable_file != NULL);
    assert(strcmp(options.executable_file, "myfile") == 0);

    arena_free(&options.arena);
}

static void test_default_output_file_unused(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "myfile.sn"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 2);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.output_file == NULL);

    arena_free(&options.arena);
}

static void test_emit_c_output_path(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "myfile.sn", "--emit-c"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 3);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    /* In emit-c mode, output should be .c file */
    assert(options.target == TARGET_C);
    assert(options.output_kind == OUTPUT_SOURCE);
    assert(options.output_file != NULL);
    assert(strcmp(options.output_file, "myfile.c") == 0);
    assert(options.executable_file == NULL);

    arena_free(&options.arena);
}

static void test_emit_rust_output_path(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "myfile.sn", "--target", "rust", "--emit-source"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 5);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.target == TARGET_RUST);
    assert(options.output_kind == OUTPUT_SOURCE);
    assert(options.output_file != NULL);
    assert(strcmp(options.output_file, "myfile.rs") == 0);
    assert(options.executable_file == NULL);

    arena_free(&options.arena);
}

/* ============================================================================
 * Target Selection Tests
 * ============================================================================ */

static void test_target_default_is_c(void)
{
    /* No target flags: the compiler must keep C as the default target.
       Exercise the real compiler_init path, which is where the default
       is actually assigned (src/compiler.c:31). */
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    /* Poison only fields compiler_init must explicitly reset. */
    options.debug_build = 1;
    options.profile_build = 1;
    const char *args[] = {"sn", "test.sn"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 2);

    compiler_init(&options, argc, argv);
    assert(options.target == TARGET_C);
    assert(options.output_kind == OUTPUT_EXECUTABLE);
    assert(options.debug_build == 0);
    assert(options.profile_build == 0);

    compiler_cleanup(&options);
}

static void test_target_c_explicit(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "--target", "c"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 4);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.target == TARGET_C);

    arena_free(&options.arena);
}

static void test_target_rust_flag(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "--target", "rust"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 4);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.target == TARGET_RUST);

    arena_free(&options.arena);
}

static void test_target_rs_alias(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "--target", "rs"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 4);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.target == TARGET_RUST);

    arena_free(&options.arena);
}

static void test_target_duplicate_same_value(void)
{
    /* Repeating --target with the same value is not a conflict. */
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "--target", "c", "--target", "c"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 6);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.target == TARGET_C);

    arena_free(&options.arena);
}

static void test_target_unknown_rejected(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "--target", "go"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 4);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    /* Unknown target names must be rejected ("Unknown target 'go' (expected c or rust)") */
    assert(result == 0);

    arena_free(&options.arena);
}

static void test_target_missing_value_rejected(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "--target"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 3);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    /* "--target requires c or rust" */
    assert(result == 0);

    arena_free(&options.arena);
}

static void test_target_conflicting_flags(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "--target", "c", "--target", "rust"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 6);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    /* "conflicting compilation targets requested" */
    assert(result == 0);

    arena_free(&options.arena);
}

static void test_emit_rust_shorthand(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "--emit-rust"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 3);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.target == TARGET_RUST);
    assert(options.output_kind == OUTPUT_SOURCE);
    assert(options.output_file != NULL);
    assert(strcmp(options.output_file, "test.rs") == 0);
    assert(options.executable_file == NULL);

    arena_free(&options.arena);
}

static void test_emit_rust_shorthand_output_path(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "myfile.sn", "--emit-rust"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 3);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.output_file != NULL);
    assert(strcmp(options.output_file, "myfile.rs") == 0);
    assert(options.executable_file == NULL);

    arena_free(&options.arena);
}

static void test_emit_rust_conflicts_with_target_c(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "--target", "c", "--emit-rust"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 5);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    /* "--emit-rust conflicts with --target c" */
    assert(result == 0);

    arena_free(&options.arena);
}

static void test_emit_c_conflicts_with_target_rust(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "--target", "rust", "--emit-c"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 5);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    /* "--emit-c conflicts with --target rust" */
    assert(result == 0);

    arena_free(&options.arena);
}

static void test_emit_c_and_emit_rust_conflict(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "--emit-c", "--emit-rust"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 4);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    /* Second emit shorthand sets a different target than the first */
    assert(result == 0);

    arena_free(&options.arena);
}

/* ============================================================================
 * Error Handling Tests
 * ============================================================================ */

static void test_no_source_file_error(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "-v"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 2);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    /* Should return 0 (error) when no source file is specified */
    assert(result == 0);

    arena_free(&options.arena);
}

static void test_unknown_option_error(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "--unknown-option"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 3);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 0);

    arena_free(&options.arena);
}

static void test_invalid_log_level_error(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "-l", "99"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 4);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 0);

    arena_free(&options.arena);
}

/* ============================================================================
 * Multiple Flags Tests
 * ============================================================================ */

static void test_multiple_flags_combined(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "-v", "-g", "--keep-c", "-O1"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 6);

    arena_init(&options.arena, 1024);
    options.optimization_level = OPT_LEVEL_FULL;

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.verbose == 1);
    assert(options.debug_build == 1);
    assert(options.keep_generated == 1);
    assert(options.optimization_level == OPT_LEVEL_BASIC);

    arena_free(&options.arena);
}

static void test_flags_order_independence(void)
{
    CompilerOptions options1, options2;
    memset(&options1, 0, sizeof(options1));
    memset(&options2, 0, sizeof(options2));

    const char *args1[] = {"sn", "test.sn", "-v", "-g"};
    const char *args2[] = {"sn", "-g", "-v", "test.sn"};
    int argc;
    char **argv;

    arena_init(&options1.arena, 1024);
    arena_init(&options2.arena, 1024);

    make_args(&argc, &argv, args1, 4);
    int result1 = compiler_parse_args(argc, argv, &options1);

    make_args(&argc, &argv, args2, 4);
    int result2 = compiler_parse_args(argc, argv, &options2);

    assert(result1 == 1);
    assert(result2 == 1);
    assert(options1.verbose == options2.verbose);
    assert(options1.debug_build == options2.debug_build);

    arena_free(&options1.arena);
    arena_free(&options2.arena);
}

/* ============================================================================
 * Legacy Flag Tests
 * ============================================================================ */

static void test_no_opt_legacy_flag_rejected(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "--no-opt"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 3);

    arena_init(&options.arena, 1024);
    options.optimization_level = OPT_LEVEL_FULL;

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 0);

    arena_free(&options.arena);
}

/* ============================================================================
 * Edge Case Tests
 * ============================================================================ */

static void test_source_file_without_extension(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "myfile"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 2);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.source_file != NULL);
    assert(strcmp(options.source_file, "myfile") == 0);

    arena_free(&options.arena);
}

static void test_output_with_path(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "-o", "build/out/program"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 4);

    arena_init(&options.arena, 1024);

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.executable_file != NULL);
    assert(strcmp(options.executable_file, "build/out/program") == 0);

    arena_free(&options.arena);
}

/* ============================================================================
 * Stress Tests
 * ============================================================================ */

static void test_repeated_parsing(void)
{
    for (int i = 0; i < 50; i++) {
        CompilerOptions options;
        memset(&options, 0, sizeof(options));
        const char *args[] = {"sn", "test.sn", "-v"};
        int argc;
        char **argv;
        make_args(&argc, &argv, args, 3);

        arena_init(&options.arena, 1024);

        int result = compiler_parse_args(argc, argv, &options);
        assert(result == 1);

        arena_free(&options.arena);
    }
}

static void test_many_flags(void)
{
    CompilerOptions options;
    memset(&options, 0, sizeof(options));
    const char *args[] = {"sn", "test.sn", "-v", "-g", "--keep-c", "-O2", "--unchecked", "-l", "2"};
    int argc;
    char **argv;
    make_args(&argc, &argv, args, 9);

    arena_init(&options.arena, 1024);
    options.log_level = DEBUG_LEVEL_ERROR;

    int result = compiler_parse_args(argc, argv, &options);
    assert(result == 1);
    assert(options.verbose == 1);
    assert(options.debug_build == 1);
    assert(options.keep_generated == 1);
    assert(options.optimization_level == OPT_LEVEL_FULL);
    assert(options.arithmetic_mode == ARITH_UNCHECKED);
    assert(options.log_level == DEBUG_LEVEL_WARNING);

    arena_free(&options.arena);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

void test_compiler_driver_main(void)
{
    TEST_SECTION("Compiler Driver - Optimization Levels");
    TEST_RUN("opt_level_default", test_opt_level_default);
    TEST_RUN("opt_level_O0", test_opt_level_O0);
    TEST_RUN("opt_level_O1", test_opt_level_O1);
    TEST_RUN("opt_level_O2", test_opt_level_O2);

    TEST_SECTION("Compiler Driver - Arithmetic Mode");
    TEST_RUN("arith_default_checked", test_arith_default_checked);
    TEST_RUN("arith_unchecked_flag", test_arith_unchecked_flag);
    TEST_RUN("arith_checked_flag", test_arith_checked_flag);
    TEST_RUN("arith_O2_explicit_defaults_unchecked", test_arith_O2_explicit_defaults_unchecked);
    TEST_RUN("arith_O2_with_checked_override", test_arith_O2_with_checked_override);

    TEST_SECTION("Compiler Driver - Output Options");
    TEST_RUN("output_file_flag", test_output_file_flag);
    TEST_RUN("emit_c_flag", test_emit_c_flag);
    TEST_RUN("keep_c_flag", test_keep_c_flag);

    TEST_SECTION("Compiler Driver - Debug Options");
    TEST_RUN("verbose_flag", test_verbose_flag);
    TEST_RUN("debug_flag", test_debug_flag);
    TEST_RUN("log_level_flag", test_log_level_flag);
    TEST_RUN("log_level_verbose", test_log_level_verbose);

    TEST_SECTION("Compiler Driver - Source File");
    TEST_RUN("source_file_parsed", test_source_file_parsed);
    TEST_RUN("source_file_path", test_source_file_path);

    TEST_SECTION("Compiler Driver - Output Path Derivation");
    TEST_RUN("default_output_path", test_default_output_path);
    TEST_RUN("default_output_file_unused", test_default_output_file_unused);
    TEST_RUN("emit_c_output_path", test_emit_c_output_path);
    TEST_RUN("emit_rust_output_path", test_emit_rust_output_path);

    TEST_SECTION("Compiler Driver - Target Selection");
    TEST_RUN("target_default_c", test_target_default_is_c);
    TEST_RUN("target_c_explicit", test_target_c_explicit);
    TEST_RUN("target_rust_flag", test_target_rust_flag);
    TEST_RUN("target_rs_alias", test_target_rs_alias);
    TEST_RUN("target_duplicate_same_value", test_target_duplicate_same_value);
    TEST_RUN("target_unknown_rejected", test_target_unknown_rejected);
    TEST_RUN("target_missing_value_rejected", test_target_missing_value_rejected);
    TEST_RUN("target_conflicting_flags", test_target_conflicting_flags);
    TEST_RUN("emit_rust_shorthand", test_emit_rust_shorthand);
    TEST_RUN("emit_rust_shorthand_output_path", test_emit_rust_shorthand_output_path);
    TEST_RUN("emit_rust_conflicts_with_target_c", test_emit_rust_conflicts_with_target_c);
    TEST_RUN("emit_c_conflicts_with_target_rust", test_emit_c_conflicts_with_target_rust);
    TEST_RUN("emit_c_and_emit_rust_conflict", test_emit_c_and_emit_rust_conflict);

    TEST_SECTION("Compiler Driver - Error Handling");
    TEST_RUN("no_source_file_error", test_no_source_file_error);
    TEST_RUN("unknown_option_error", test_unknown_option_error);
    TEST_RUN("invalid_log_level_error", test_invalid_log_level_error);

    TEST_SECTION("Compiler Driver - Multiple Flags");
    TEST_RUN("multiple_flags_combined", test_multiple_flags_combined);
    TEST_RUN("flags_order_independence", test_flags_order_independence);

    TEST_SECTION("Compiler Driver - Legacy Flags");
    TEST_RUN("no_opt_legacy_flag_rejected", test_no_opt_legacy_flag_rejected);

    TEST_SECTION("Compiler Driver - Edge Cases");
    TEST_RUN("source_file_without_extension", test_source_file_without_extension);
    TEST_RUN("output_with_path", test_output_with_path);

    TEST_SECTION("Compiler Driver - Stress Tests");
    TEST_RUN("repeated_parsing", test_repeated_parsing);
    TEST_RUN("many_flags", test_many_flags);
}
