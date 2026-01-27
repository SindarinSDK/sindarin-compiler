#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../managed_arena.h"

/* Portable microsecond sleep for tests */
#define usleep(us) rt_arena_sleep_ms(((us) + 999) / 1000)

/* ============================================================================
 * ANSI color codes (matching unit test harness)
 * ============================================================================ */
#define TEST_COLOR_GREEN  "\033[0;32m"
#define TEST_COLOR_RED    "\033[0;31m"
#define TEST_COLOR_BOLD   "\033[1m"
#define TEST_COLOR_RESET  "\033[0m"

/* ============================================================================
 * High-resolution timer
 * ============================================================================ */
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

static inline double test_timer_now(void) {
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart / (double)freq.QuadPart;
}
#else
#include <time.h>

static inline double test_timer_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}
#endif

/* ============================================================================
 * Test counters and timing (defined in test_main.c)
 * ============================================================================ */
extern int tests_passed;
extern int tests_failed;
extern double tests_total_ms;
extern char test_stats_buffer[256];

/* Macro for tests to report stats (printed after PASS on separate line) */
#define TEST_STATS(fmt, ...) \
    snprintf(test_stats_buffer, sizeof(test_stats_buffer), fmt, ##__VA_ARGS__)

/* ============================================================================
 * Assertions
 * ============================================================================ */
#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "  FAIL: %s (line %d): %s\n", __func__, __LINE__, msg); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b, msg) do { \
    if ((a) != (b)) { \
        fprintf(stderr, "  FAIL: %s (line %d): %s (got %ld, expected %ld)\n", \
                __func__, __LINE__, msg, (long)(a), (long)(b)); \
        tests_failed++; \
        return; \
    } \
} while(0)

/* ============================================================================
 * Section header (matching unit test harness)
 * ============================================================================ */
#define TEST_SECTION(name) do { \
    printf("\n" TEST_COLOR_BOLD "%s" TEST_COLOR_RESET "\n", name); \
    printf("------------------------------------------------------------\n"); \
} while(0)

/* ============================================================================
 * Test runner with per-test timing (matching unit test harness format)
 * ============================================================================ */
#define TEST_RUN(name, func) do { \
    printf("  %-50s ", name); \
    fflush(stdout); \
    double _t0 = test_timer_now(); \
    func(); \
    double _elapsed_ms = (test_timer_now() - _t0) * 1000.0; \
    tests_total_ms += _elapsed_ms; \
    if (_elapsed_ms >= 1000.0) \
        printf(TEST_COLOR_GREEN "PASS" TEST_COLOR_RESET "  (%.2fs)\n", _elapsed_ms / 1000.0); \
    else \
        printf(TEST_COLOR_GREEN "PASS" TEST_COLOR_RESET "  (%.2fms)\n", _elapsed_ms); \
    if (test_stats_buffer[0] != '\0') { \
        printf("    %s\n", test_stats_buffer); \
        test_stats_buffer[0] = '\0'; \
    } \
    tests_passed++; \
} while(0)

#endif /* TEST_FRAMEWORK_H */
