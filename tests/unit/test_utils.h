#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Test assertion macros with helpful error messages.
 * These provide better debugging information than bare assert().
 */

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "\n  ASSERTION FAILED: %s\n", msg); \
        fprintf(stderr, "    Condition: %s\n", #cond); \
        fprintf(stderr, "    Location: %s:%d\n", __FILE__, __LINE__); \
        assert(0); \
    } \
} while(0)

#define TEST_ASSERT_EQ(actual, expected, msg) do { \
    if ((actual) != (expected)) { \
        fprintf(stderr, "\n  ASSERTION FAILED: %s\n", msg); \
        fprintf(stderr, "    Expected: %s = %ld\n", #expected, (long)(expected)); \
        fprintf(stderr, "    Actual:   %s = %ld\n", #actual, (long)(actual)); \
        fprintf(stderr, "    Location: %s:%d\n", __FILE__, __LINE__); \
        assert(0); \
    } \
} while(0)

#define TEST_ASSERT_STR_EQ(actual, expected, msg) do { \
    const char *_a = (actual); \
    const char *_e = (expected); \
    if (_a == NULL && _e == NULL) { /* OK */ } \
    else if (_a == NULL || _e == NULL || strcmp(_a, _e) != 0) { \
        fprintf(stderr, "\n  ASSERTION FAILED: %s\n", msg); \
        fprintf(stderr, "    Expected: \"%s\"\n", _e ? _e : "(null)"); \
        fprintf(stderr, "    Actual:   \"%s\"\n", _a ? _a : "(null)"); \
        fprintf(stderr, "    Location: %s:%d\n", __FILE__, __LINE__); \
        assert(0); \
    } \
} while(0)

#define TEST_ASSERT_NOT_NULL(ptr, msg) do { \
    if ((ptr) == NULL) { \
        fprintf(stderr, "\n  ASSERTION FAILED: %s\n", msg); \
        fprintf(stderr, "    Expected: non-NULL\n"); \
        fprintf(stderr, "    Actual:   NULL\n"); \
        fprintf(stderr, "    Location: %s:%d\n", __FILE__, __LINE__); \
        assert(0); \
    } \
} while(0)

#define TEST_ASSERT_NULL(ptr, msg) do { \
    if ((ptr) != NULL) { \
        fprintf(stderr, "\n  ASSERTION FAILED: %s\n", msg); \
        fprintf(stderr, "    Expected: NULL\n"); \
        fprintf(stderr, "    Actual:   non-NULL (%p)\n", (void*)(ptr)); \
        fprintf(stderr, "    Location: %s:%d\n", __FILE__, __LINE__); \
        assert(0); \
    } \
} while(0)

#define TEST_ASSERT_TRUE(cond, msg) TEST_ASSERT((cond), msg)
#define TEST_ASSERT_FALSE(cond, msg) TEST_ASSERT(!(cond), msg)

/*
 * Test lifecycle macros
 */
#define TEST_BEGIN(name) \
    printf("Testing %s...\n", name); \
    DEBUG_INFO("Starting %s", name)

#define TEST_END(name) \
    DEBUG_INFO("Finished %s", name)

/*
 * Common runtime header string for code generation tests.
 * Matches the output of code_gen_headers() + code_gen_externs().
 */
static const char *CODE_GEN_RUNTIME_HEADER =
    "#include <stdlib.h>\n"
    "#include <string.h>\n"
    "#include <stdio.h>\n"
    "#include <stdbool.h>\n"
    "#include <stdint.h>\n"
    "#include <limits.h>\n"
    "#include <setjmp.h>\n"
    "#include \"runtime.h\"\n"
    "#ifdef _WIN32\n"
    "#undef min\n"
    "#undef max\n"
    "#endif\n"
    "\n"
    "\n"
    "/* Closure type for lambdas */\n"
    "typedef struct __Closure__ { void *fn; RtArena *arena; size_t size; } __Closure__;\n"
    "\n"
    "/* Forward declarations */\n"
    "static RtManagedArena *__main_arena__ = NULL;\n"
    "\n";

/*
 * Helper to build expected code gen output.
 * Combines runtime header with test-specific code.
 */
static inline char *build_expected_output(Arena *arena, const char *code)
{
    size_t header_len = strlen(CODE_GEN_RUNTIME_HEADER);
    size_t code_len = strlen(code);
    char *result = arena_alloc(arena, header_len + code_len + 1);
    if (result == NULL) {
        fprintf(stderr, "build_expected_output: allocation failed\n");
        exit(1);
    }
    memcpy(result, CODE_GEN_RUNTIME_HEADER, header_len);
    memcpy(result + header_len, code, code_len + 1);
    return result;
}

#endif /* TEST_UTILS_H */
