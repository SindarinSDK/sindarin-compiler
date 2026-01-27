// tests/unit/standalone/diagnostic_tests.c
// Unit tests for the diagnostic system

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../test_harness.h"
#include "../diagnostic.h"

/* ============================================================================
 * Initialization Tests
 * ============================================================================ */

static void test_diagnostic_init_basic(void)
{
    diagnostic_init("test.sn", "var x: int = 42");
    assert(diagnostic_error_count() == 0);
    assert(diagnostic_warning_count() == 0);
    assert(diagnostic_had_error() == 0);
}

static void test_diagnostic_init_null_source(void)
{
    diagnostic_init("test.sn", NULL);
    assert(diagnostic_error_count() == 0);
}

static void test_diagnostic_init_empty_source(void)
{
    diagnostic_init("test.sn", "");
    assert(diagnostic_error_count() == 0);
}

static void test_diagnostic_init_null_filename(void)
{
    diagnostic_init(NULL, "source code");
    assert(diagnostic_error_count() == 0);
}

static void test_diagnostic_init_multiline_source(void)
{
    const char *source = "line1\nline2\nline3\n";
    diagnostic_init("test.sn", source);
    assert(diagnostic_error_count() == 0);
}

/* ============================================================================
 * Reset Tests
 * ============================================================================ */

static void test_diagnostic_reset_clears_errors(void)
{
    diagnostic_init("test.sn", "var x = 1");
    diagnostic_error_simple("test error 1");
    diagnostic_error_simple("test error 2");
    assert(diagnostic_error_count() == 2);

    diagnostic_reset();
    assert(diagnostic_error_count() == 0);
    assert(diagnostic_had_error() == 0);
}

static void test_diagnostic_reset_clears_warnings(void)
{
    diagnostic_init("test.sn", "var x = 1");
    Token tok = {.filename = "test.sn", .line = 1, .length = 1, .start = NULL};
    diagnostic_warning_at(&tok, "test warning");
    assert(diagnostic_warning_count() == 1);

    diagnostic_reset();
    assert(diagnostic_warning_count() == 0);
}

static void test_diagnostic_reset_multiple_times(void)
{
    diagnostic_init("test.sn", "source");
    diagnostic_reset();
    diagnostic_reset();
    diagnostic_reset();
    assert(diagnostic_error_count() == 0);
}

/* ============================================================================
 * Error Count Tests
 * ============================================================================ */

static void test_error_count_increments(void)
{
    diagnostic_init("test.sn", "source");
    assert(diagnostic_error_count() == 0);

    diagnostic_error_simple("error 1");
    assert(diagnostic_error_count() == 1);

    diagnostic_error_simple("error 2");
    assert(diagnostic_error_count() == 2);

    diagnostic_error_simple("error 3");
    assert(diagnostic_error_count() == 3);
}

static void test_had_error_false_initially(void)
{
    diagnostic_init("test.sn", "source");
    assert(diagnostic_had_error() == 0);
}

static void test_had_error_true_after_error(void)
{
    diagnostic_init("test.sn", "source");
    diagnostic_error_simple("an error");
    assert(diagnostic_had_error() == 1);
}

static void test_had_error_stays_true(void)
{
    diagnostic_init("test.sn", "source");
    diagnostic_error_simple("error");
    assert(diagnostic_had_error() == 1);
    /* Still true after checking */
    assert(diagnostic_had_error() == 1);
}

/* ============================================================================
 * Warning Count Tests
 * ============================================================================ */

static void test_warning_count_increments(void)
{
    diagnostic_init("test.sn", "source");
    Token tok = {.filename = "test.sn", .line = 1, .length = 1, .start = NULL};

    diagnostic_warning_at(&tok, "warning 1");
    assert(diagnostic_warning_count() == 1);

    diagnostic_warning_at(&tok, "warning 2");
    assert(diagnostic_warning_count() == 2);
}

static void test_warnings_dont_affect_error_count(void)
{
    diagnostic_init("test.sn", "source");
    Token tok = {.filename = "test.sn", .line = 1, .length = 1, .start = NULL};

    diagnostic_warning_at(&tok, "warning");
    assert(diagnostic_error_count() == 0);
    assert(diagnostic_had_error() == 0);
}

/* ============================================================================
 * DiagnosticLoc Tests
 * ============================================================================ */

static void test_loc_from_token_basic(void)
{
    Token tok = {
        .filename = "test.sn",
        .line = 10,
        .length = 5,
        .start = NULL
    };

    DiagnosticLoc loc = diagnostic_loc_from_token(&tok);
    assert(loc.filename != NULL);
    assert(strcmp(loc.filename, "test.sn") == 0);
    assert(loc.line == 10);
    assert(loc.length == 5);
}

static void test_loc_from_token_null(void)
{
    DiagnosticLoc loc = diagnostic_loc_from_token(NULL);
    assert(loc.filename == NULL);
    assert(loc.line == 0);
    assert(loc.column == 0);
    assert(loc.length == 0);
}

static void test_loc_column_computed(void)
{
    const char *source = "var x = 42";
    diagnostic_init("test.sn", source);

    Token tok = {
        .filename = "test.sn",
        .line = 1,
        .length = 1,
        .start = source + 4  /* Points to 'x' */
    };

    DiagnosticLoc loc = diagnostic_loc_from_token(&tok);
    assert(loc.column == 5);  /* 'x' is at column 5 (1-indexed) */
}

static void test_loc_column_start_of_line(void)
{
    const char *source = "var x = 42";
    diagnostic_init("test.sn", source);

    Token tok = {
        .filename = "test.sn",
        .line = 1,
        .length = 3,
        .start = source  /* Points to 'var' */
    };

    DiagnosticLoc loc = diagnostic_loc_from_token(&tok);
    assert(loc.column == 1);
}

static void test_loc_multiline_source(void)
{
    const char *source = "line1\nvar x = 42";
    diagnostic_init("test.sn", source);

    Token tok = {
        .filename = "test.sn",
        .line = 2,
        .length = 1,
        .start = source + 6 + 4  /* Points to 'x' on line 2 */
    };

    DiagnosticLoc loc = diagnostic_loc_from_token(&tok);
    assert(loc.line == 2);
}

/* ============================================================================
 * Verbose Mode Tests
 * ============================================================================ */

static void test_set_verbose_on(void)
{
    diagnostic_set_verbose(1);
    /* No crash means success */
}

static void test_set_verbose_off(void)
{
    diagnostic_set_verbose(0);
    /* No crash means success */
}

static void test_set_verbose_toggle(void)
{
    diagnostic_set_verbose(1);
    diagnostic_set_verbose(0);
    diagnostic_set_verbose(1);
    diagnostic_set_verbose(0);
}

/* ============================================================================
 * Error Reporting Tests
 * ============================================================================ */

static void test_error_simple(void)
{
    diagnostic_init("test.sn", "source");
    int before = diagnostic_error_count();
    diagnostic_error_simple("simple error message");
    assert(diagnostic_error_count() == before + 1);
}

static void test_error_simple_with_format(void)
{
    diagnostic_init("test.sn", "source");
    diagnostic_error_simple("error: %s at line %d", "undefined variable", 10);
    assert(diagnostic_error_count() == 1);
}

static void test_error_at_token(void)
{
    const char *source = "var x = undefined";
    diagnostic_init("test.sn", source);

    Token tok = {
        .filename = "test.sn",
        .line = 1,
        .length = 9,
        .start = source + 8
    };

    diagnostic_error_at(&tok, "undefined identifier");
    assert(diagnostic_error_count() == 1);
}

static void test_error_at_null_token(void)
{
    diagnostic_init("test.sn", "source");
    diagnostic_error_at(NULL, "error with null token");
    assert(diagnostic_error_count() == 1);
}

static void test_error_with_suggestion(void)
{
    const char *source = "var x = pritn()";
    diagnostic_init("test.sn", source);

    Token tok = {
        .filename = "test.sn",
        .line = 1,
        .length = 5,
        .start = source + 8
    };

    diagnostic_error_with_suggestion(&tok, "print", "unknown function '%s'", "pritn");
    assert(diagnostic_error_count() == 1);
}

static void test_error_with_null_suggestion(void)
{
    const char *source = "var x = unknown";
    diagnostic_init("test.sn", source);

    Token tok = {
        .filename = "test.sn",
        .line = 1,
        .length = 7,
        .start = source + 8
    };

    diagnostic_error_with_suggestion(&tok, NULL, "unknown identifier");
    assert(diagnostic_error_count() == 1);
}

/* ============================================================================
 * Warning Reporting Tests
 * ============================================================================ */

static void test_warning_at_token(void)
{
    const char *source = "var x = 42";
    diagnostic_init("test.sn", source);

    Token tok = {
        .filename = "test.sn",
        .line = 1,
        .length = 1,
        .start = source + 4
    };

    diagnostic_warning_at(&tok, "unused variable 'x'");
    assert(diagnostic_warning_count() == 1);
    assert(diagnostic_error_count() == 0);
}

static void test_warning_at_null_token(void)
{
    diagnostic_init("test.sn", "source");
    diagnostic_warning_at(NULL, "warning with null token");
    assert(diagnostic_warning_count() == 1);
}

/* ============================================================================
 * Note Reporting Tests
 * ============================================================================ */

static void test_note_at_token(void)
{
    const char *source = "fn foo() {}";
    diagnostic_init("test.sn", source);

    Token tok = {
        .filename = "test.sn",
        .line = 1,
        .length = 3,
        .start = source + 3
    };

    /* Notes don't increment error or warning count */
    int errors_before = diagnostic_error_count();
    int warnings_before = diagnostic_warning_count();

    diagnostic_note_at(&tok, "function defined here");

    assert(diagnostic_error_count() == errors_before);
    assert(diagnostic_warning_count() == warnings_before);
}

static void test_note_at_null_token(void)
{
    diagnostic_init("test.sn", "source");
    diagnostic_note_at(NULL, "note with null token");
    /* Should not crash */
}

/* ============================================================================
 * diagnostic_error() Tests
 * ============================================================================ */

static void test_diagnostic_error_with_location(void)
{
    diagnostic_init("test.sn", "var x = 42");
    diagnostic_error("test.sn", 1, 5, 1, "unexpected token");
    assert(diagnostic_error_count() == 1);
}

static void test_diagnostic_error_null_filename(void)
{
    diagnostic_init("test.sn", "source");
    diagnostic_error(NULL, 1, 1, 1, "error without filename");
    assert(diagnostic_error_count() == 1);
}

static void test_diagnostic_error_zero_line(void)
{
    diagnostic_init("test.sn", "source");
    diagnostic_error("test.sn", 0, 0, 0, "error at unknown location");
    assert(diagnostic_error_count() == 1);
}

static void test_diagnostic_error_large_line(void)
{
    diagnostic_init("test.sn", "source");
    diagnostic_error("test.sn", 9999, 1, 1, "error at large line number");
    assert(diagnostic_error_count() == 1);
}

/* ============================================================================
 * diagnostic_report() Tests
 * ============================================================================ */

static void test_report_error_level(void)
{
    diagnostic_init("test.sn", "source");
    DiagnosticLoc loc = {"test.sn", 1, 1, 1};
    diagnostic_report(DIAG_ERROR, loc, "error message");
    assert(diagnostic_error_count() == 1);
}

static void test_report_warning_level(void)
{
    diagnostic_init("test.sn", "source");
    DiagnosticLoc loc = {"test.sn", 1, 1, 1};
    diagnostic_report(DIAG_WARNING, loc, "warning message");
    assert(diagnostic_warning_count() == 1);
}

static void test_report_note_level(void)
{
    diagnostic_init("test.sn", "source");
    DiagnosticLoc loc = {"test.sn", 1, 1, 1};
    int errors = diagnostic_error_count();
    int warnings = diagnostic_warning_count();
    diagnostic_report(DIAG_NOTE, loc, "note message");
    assert(diagnostic_error_count() == errors);
    assert(diagnostic_warning_count() == warnings);
}

static void test_report_invalid_level(void)
{
    diagnostic_init("test.sn", "source");
    DiagnosticLoc loc = {"test.sn", 1, 1, 1};
    /* Invalid level should default to error */
    diagnostic_report((DiagnosticLevel)999, loc, "invalid level");
    /* Should still work without crashing */
}

/* ============================================================================
 * Phase Reporting Tests
 * ============================================================================ */

static void test_phase_parsing(void)
{
    diagnostic_init("test.sn", "source");
    diagnostic_phase_start(PHASE_PARSING);
    diagnostic_phase_done(PHASE_PARSING, 0.0);
}

static void test_phase_type_check(void)
{
    diagnostic_init("test.sn", "source");
    diagnostic_phase_start(PHASE_TYPE_CHECK);
    diagnostic_phase_done(PHASE_TYPE_CHECK, 0.0);
}

static void test_phase_code_gen(void)
{
    diagnostic_init("test.sn", "source");
    diagnostic_phase_start(PHASE_CODE_GEN);
    diagnostic_phase_done(PHASE_CODE_GEN, 0.0);
}

static void test_phase_linking(void)
{
    diagnostic_init("test.sn", "source");
    diagnostic_phase_start(PHASE_LINKING);
    diagnostic_phase_done(PHASE_LINKING, 0.0);
}

static void test_phase_failed(void)
{
    diagnostic_init("test.sn", "source");
    diagnostic_phase_start(PHASE_PARSING);
    diagnostic_phase_failed(PHASE_PARSING);
}

static void test_phase_with_timing(void)
{
    diagnostic_set_verbose(1);
    diagnostic_init("test.sn", "source");
    diagnostic_phase_start(PHASE_PARSING);
    diagnostic_phase_done(PHASE_PARSING, 0.5);
    diagnostic_set_verbose(0);
}

/* ============================================================================
 * Compile Start/Success/Failed Tests
 * ============================================================================ */

static void test_compile_start(void)
{
    diagnostic_compile_start("test.sn");
    /* Should not crash */
}

static void test_compile_success(void)
{
    diagnostic_compile_success("output.exe", 12345, 0.5);
}

static void test_compile_success_large_file(void)
{
    diagnostic_compile_success("output.exe", 1024 * 1024 * 10, 1.5);  /* 10 MB */
}

static void test_compile_success_small_file(void)
{
    diagnostic_compile_success("output.exe", 100, 0.1);  /* 100 bytes */
}

static void test_compile_success_verbose(void)
{
    diagnostic_set_verbose(1);
    diagnostic_compile_success("output.exe", 1024 * 100, 0.5);
    diagnostic_set_verbose(0);
}

static void test_compile_failed(void)
{
    diagnostic_init("test.sn", "source");
    diagnostic_compile_failed();
}

static void test_compile_failed_with_errors(void)
{
    diagnostic_init("test.sn", "source");
    diagnostic_error_simple("error 1");
    diagnostic_error_simple("error 2");
    diagnostic_compile_failed();
}

/* ============================================================================
 * Edge Case Tests
 * ============================================================================ */

static void test_many_errors(void)
{
    diagnostic_init("test.sn", "source");
    for (int i = 0; i < 100; i++) {
        diagnostic_error_simple("error %d", i);
    }
    assert(diagnostic_error_count() == 100);
}

static void test_many_warnings(void)
{
    diagnostic_init("test.sn", "source");
    Token tok = {.filename = "test.sn", .line = 1, .length = 1, .start = NULL};
    for (int i = 0; i < 100; i++) {
        diagnostic_warning_at(&tok, "warning %d", i);
    }
    assert(diagnostic_warning_count() == 100);
}

static void test_mixed_errors_and_warnings(void)
{
    diagnostic_init("test.sn", "source");
    Token tok = {.filename = "test.sn", .line = 1, .length = 1, .start = NULL};

    for (int i = 0; i < 50; i++) {
        diagnostic_error_simple("error %d", i);
        diagnostic_warning_at(&tok, "warning %d", i);
    }

    assert(diagnostic_error_count() == 50);
    assert(diagnostic_warning_count() == 50);
}

static void test_long_error_message(void)
{
    diagnostic_init("test.sn", "source");
    char msg[512];
    memset(msg, 'x', sizeof(msg) - 1);
    msg[sizeof(msg) - 1] = '\0';
    diagnostic_error_simple("%s", msg);
    assert(diagnostic_error_count() == 1);
}

static void test_special_chars_in_message(void)
{
    diagnostic_init("test.sn", "source");
    diagnostic_error_simple("error with special chars: <>\"'&%%");
    assert(diagnostic_error_count() == 1);
}

/* ============================================================================
 * Reinit Tests
 * ============================================================================ */

static void test_reinit_clears_counts(void)
{
    diagnostic_init("test1.sn", "source1");
    diagnostic_error_simple("error");
    assert(diagnostic_error_count() == 1);

    diagnostic_init("test2.sn", "source2");
    assert(diagnostic_error_count() == 0);
}

static void test_reinit_with_different_source(void)
{
    diagnostic_init("test.sn", "short");
    diagnostic_init("test.sn", "much longer source code here");
    assert(diagnostic_error_count() == 0);
}

/* ============================================================================
 * Stress Tests
 * ============================================================================ */

static void test_repeated_init(void)
{
    for (int i = 0; i < 100; i++) {
        diagnostic_init("test.sn", "source");
        assert(diagnostic_error_count() == 0);
    }
}

static void test_repeated_reset(void)
{
    diagnostic_init("test.sn", "source");
    for (int i = 0; i < 100; i++) {
        diagnostic_error_simple("error");
        diagnostic_reset();
        assert(diagnostic_error_count() == 0);
    }
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

void test_diagnostic_main(void)
{
    TEST_SECTION("Diagnostic - Initialization");
    TEST_RUN("init_basic", test_diagnostic_init_basic);
    TEST_RUN("init_null_source", test_diagnostic_init_null_source);
    TEST_RUN("init_empty_source", test_diagnostic_init_empty_source);
    TEST_RUN("init_null_filename", test_diagnostic_init_null_filename);
    TEST_RUN("init_multiline_source", test_diagnostic_init_multiline_source);

    TEST_SECTION("Diagnostic - Reset");
    TEST_RUN("reset_clears_errors", test_diagnostic_reset_clears_errors);
    TEST_RUN("reset_clears_warnings", test_diagnostic_reset_clears_warnings);
    TEST_RUN("reset_multiple_times", test_diagnostic_reset_multiple_times);

    TEST_SECTION("Diagnostic - Error Count");
    TEST_RUN("error_count_increments", test_error_count_increments);
    TEST_RUN("had_error_false_initially", test_had_error_false_initially);
    TEST_RUN("had_error_true_after_error", test_had_error_true_after_error);
    TEST_RUN("had_error_stays_true", test_had_error_stays_true);

    TEST_SECTION("Diagnostic - Warning Count");
    TEST_RUN("warning_count_increments", test_warning_count_increments);
    TEST_RUN("warnings_dont_affect_error_count", test_warnings_dont_affect_error_count);

    TEST_SECTION("Diagnostic - Location");
    TEST_RUN("loc_from_token_basic", test_loc_from_token_basic);
    TEST_RUN("loc_from_token_null", test_loc_from_token_null);
    TEST_RUN("loc_column_computed", test_loc_column_computed);
    TEST_RUN("loc_column_start_of_line", test_loc_column_start_of_line);
    TEST_RUN("loc_multiline_source", test_loc_multiline_source);

    TEST_SECTION("Diagnostic - Verbose Mode");
    TEST_RUN("set_verbose_on", test_set_verbose_on);
    TEST_RUN("set_verbose_off", test_set_verbose_off);
    TEST_RUN("set_verbose_toggle", test_set_verbose_toggle);

    TEST_SECTION("Diagnostic - Error Reporting");
    TEST_RUN("error_simple", test_error_simple);
    TEST_RUN("error_simple_with_format", test_error_simple_with_format);
    TEST_RUN("error_at_token", test_error_at_token);
    TEST_RUN("error_at_null_token", test_error_at_null_token);
    TEST_RUN("error_with_suggestion", test_error_with_suggestion);
    TEST_RUN("error_with_null_suggestion", test_error_with_null_suggestion);

    TEST_SECTION("Diagnostic - Warning Reporting");
    TEST_RUN("warning_at_token", test_warning_at_token);
    TEST_RUN("warning_at_null_token", test_warning_at_null_token);

    TEST_SECTION("Diagnostic - Note Reporting");
    TEST_RUN("note_at_token", test_note_at_token);
    TEST_RUN("note_at_null_token", test_note_at_null_token);

    TEST_SECTION("Diagnostic - diagnostic_error()");
    TEST_RUN("error_with_location", test_diagnostic_error_with_location);
    TEST_RUN("error_null_filename", test_diagnostic_error_null_filename);
    TEST_RUN("error_zero_line", test_diagnostic_error_zero_line);
    TEST_RUN("error_large_line", test_diagnostic_error_large_line);

    TEST_SECTION("Diagnostic - diagnostic_report()");
    TEST_RUN("report_error_level", test_report_error_level);
    TEST_RUN("report_warning_level", test_report_warning_level);
    TEST_RUN("report_note_level", test_report_note_level);
    TEST_RUN("report_invalid_level", test_report_invalid_level);

    TEST_SECTION("Diagnostic - Phase Reporting");
    TEST_RUN("phase_parsing", test_phase_parsing);
    TEST_RUN("phase_type_check", test_phase_type_check);
    TEST_RUN("phase_code_gen", test_phase_code_gen);
    TEST_RUN("phase_linking", test_phase_linking);
    TEST_RUN("phase_failed", test_phase_failed);
    TEST_RUN("phase_with_timing", test_phase_with_timing);

    TEST_SECTION("Diagnostic - Compile Status");
    TEST_RUN("compile_start", test_compile_start);
    TEST_RUN("compile_success", test_compile_success);
    TEST_RUN("compile_success_large_file", test_compile_success_large_file);
    TEST_RUN("compile_success_small_file", test_compile_success_small_file);
    TEST_RUN("compile_success_verbose", test_compile_success_verbose);
    TEST_RUN("compile_failed", test_compile_failed);
    TEST_RUN("compile_failed_with_errors", test_compile_failed_with_errors);

    TEST_SECTION("Diagnostic - Edge Cases");
    TEST_RUN("many_errors", test_many_errors);
    TEST_RUN("many_warnings", test_many_warnings);
    TEST_RUN("mixed_errors_and_warnings", test_mixed_errors_and_warnings);
    TEST_RUN("long_error_message", test_long_error_message);
    TEST_RUN("special_chars_in_message", test_special_chars_in_message);

    TEST_SECTION("Diagnostic - Reinit");
    TEST_RUN("reinit_clears_counts", test_reinit_clears_counts);
    TEST_RUN("reinit_with_different_source", test_reinit_with_different_source);

    TEST_SECTION("Diagnostic - Stress Tests");
    TEST_RUN("repeated_init", test_repeated_init);
    TEST_RUN("repeated_reset", test_repeated_reset);
}
