#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../managed_arena.h"

/* Portable microsecond sleep for tests */
#define usleep(us) rt_arena_sleep_ms(((us) + 999) / 1000)

/* Shared counters (defined in test_main.c) */
extern int tests_passed;
extern int tests_failed;

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

#define TEST_RUN(name, func) do { \
    printf("  %-50s", name); \
    func(); \
    printf("PASS\n"); \
    tests_passed++; \
} while(0)

#endif /* TEST_FRAMEWORK_H */
