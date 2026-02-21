// test_harness.h - Unified test output formatting
// Provides consistent output format matching integration/exploratory tests

#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdio.h>
#include <string.h>

// Cross-platform high-resolution timing
#ifdef _WIN32
#include <windows.h>
static double _test_get_time_ms(void) {
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart * 1000.0 / (double)freq.QuadPart;
}
#else
#include <sys/time.h>
static double _test_get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}
#endif

// ANSI color codes
#define TEST_COLOR_GREEN  "\033[0;32m"
#define TEST_COLOR_RED    "\033[0;31m"
#define TEST_COLOR_YELLOW "\033[0;33m"
#define TEST_COLOR_BOLD   "\033[1m"
#define TEST_COLOR_RESET  "\033[0m"

// Global test counters
static int _test_passed = 0;
static int _test_failed = 0;
static int _test_section_passed = 0;
static int _test_section_failed = 0;

// Timing state
static double _test_suite_start_ms = 0;
static double _test_current_start_ms = 0;


// Print section header
// Usage: TEST_SECTION("Runtime Arena");
#define TEST_SECTION(name) do { \
    _test_section_passed = 0; \
    _test_section_failed = 0; \
    printf("\n" TEST_COLOR_BOLD "%s" TEST_COLOR_RESET "\n", name); \
    printf("------------------------------------------------------------\n"); \
} while(0)

// Print section summary (optional, for long sections)
#define TEST_SECTION_END() do { \
    if (_test_section_failed > 0) { \
        printf("  Section: " TEST_COLOR_GREEN "%d passed" TEST_COLOR_RESET ", " \
               TEST_COLOR_RED "%d failed" TEST_COLOR_RESET "\n", \
               _test_section_passed, _test_section_failed); \
    } \
} while(0)

// Run a test and print result
// Usage: TEST("arena_create", { ... test code with asserts ... });
// The test code runs, and if it completes without assert failure, prints PASS
#define TEST(name, code) do { \
    printf("  %-50s ", name); \
    fflush(stdout); \
    _test_current_start_ms = _test_get_time_ms(); \
    { code } \
    double _test_elapsed = _test_get_time_ms() - _test_current_start_ms; \
    if (_test_elapsed >= 1000.0) \
        printf(TEST_COLOR_GREEN "PASS" TEST_COLOR_RESET "  (%.2fs)\n", _test_elapsed / 1000.0); \
    else \
        printf(TEST_COLOR_GREEN "PASS" TEST_COLOR_RESET "  (%.2fms)\n", _test_elapsed); \
    _test_passed++; \
    _test_section_passed++; \
} while(0)

// Simpler version: just announce test start, print PASS at end
// For tests that are already written as separate functions
#define TEST_RUN(name, func) do { \
    printf("  %-50s ", name); \
    fflush(stdout); \
    _test_current_start_ms = _test_get_time_ms(); \
    func(); \
    double _test_elapsed = _test_get_time_ms() - _test_current_start_ms; \
    if (_test_elapsed >= 1000.0) \
        printf(TEST_COLOR_GREEN "PASS" TEST_COLOR_RESET "  (%.2fs)\n", _test_elapsed / 1000.0); \
    else \
        printf(TEST_COLOR_GREEN "PASS" TEST_COLOR_RESET "  (%.2fms)\n", _test_elapsed); \
    _test_passed++; \
    _test_section_passed++; \
} while(0)

// For tests that just need the name printed (legacy support during migration)
// The test function itself should NOT print anything
#define TEST_NAME(name) do { \
    printf("  %-50s ", name); \
    fflush(stdout); \
} while(0)

#define TEST_PASS() do { \
    printf(TEST_COLOR_GREEN "PASS" TEST_COLOR_RESET "\n"); \
    _test_passed++; \
    _test_section_passed++; \
} while(0)

// Print final summary
#define TEST_SUMMARY() do { \
    double _test_total_elapsed = _test_get_time_ms() - _test_suite_start_ms; \
    printf("\n------------------------------------------------------------\n"); \
    if (_test_total_elapsed >= 1000.0) \
        printf("Results: " TEST_COLOR_GREEN "%d passed" TEST_COLOR_RESET ", " \
               TEST_COLOR_RED "%d failed" TEST_COLOR_RESET \
               "  (total: %.2fs)\n", \
               _test_passed, _test_failed, _test_total_elapsed / 1000.0); \
    else \
        printf("Results: " TEST_COLOR_GREEN "%d passed" TEST_COLOR_RESET ", " \
               TEST_COLOR_RED "%d failed" TEST_COLOR_RESET \
               "  (total: %.2fms)\n", \
               _test_passed, _test_failed, _test_total_elapsed); \
} while(0)

// Initialize test counters (call at start of main)
#define TEST_INIT() do { \
    _test_passed = 0; \
    _test_failed = 0; \
    _test_suite_start_ms = _test_get_time_ms(); \
} while(0)

// Get total passed count
#define TEST_GET_PASSED() (_test_passed)
#define TEST_GET_FAILED() (_test_failed)

#endif // TEST_HARNESS_H
