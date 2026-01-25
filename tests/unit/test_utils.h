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
    "/* Runtime string operations */\n"
    "extern char *rt_str_concat(RtArena *, const char *, const char *);\n"
    "extern long rt_str_length(const char *);\n"
    "extern char *rt_str_substring(RtArena *, const char *, long, long);\n"
    "extern long rt_str_indexOf(const char *, const char *);\n"
    "extern char **rt_str_split(RtArena *, const char *, const char *);\n"
    "extern char *rt_str_trim(RtArena *, const char *);\n"
    "extern char *rt_str_toUpper(RtArena *, const char *);\n"
    "extern char *rt_str_toLower(RtArena *, const char *);\n"
    "extern int rt_str_startsWith(const char *, const char *);\n"
    "extern int rt_str_endsWith(const char *, const char *);\n"
    "extern int rt_str_contains(const char *, const char *);\n"
    "extern char *rt_str_replace(RtArena *, const char *, const char *, const char *);\n"
    "extern long rt_str_charAt(const char *, long);\n"
    "\n"
    "/* Runtime print functions */\n"
    "extern void rt_print_long(long long);\n"
    "extern void rt_print_double(double);\n"
    "extern void rt_print_char(long);\n"
    "extern void rt_print_string(const char *);\n"
    "extern void rt_print_bool(long);\n"
    "extern void rt_print_byte(unsigned char);\n"
    "\n"
    "/* Runtime type conversions */\n"
    "extern char *rt_to_string_long(RtArena *, long long);\n"
    "extern char *rt_to_string_double(RtArena *, double);\n"
    "extern char *rt_to_string_char(RtArena *, char);\n"
    "extern char *rt_to_string_bool(RtArena *, int);\n"
    "extern char *rt_to_string_byte(RtArena *, unsigned char);\n"
    "extern char *rt_to_string_string(RtArena *, const char *);\n"
    "extern char *rt_to_string_void(RtArena *);\n"
    "extern char *rt_to_string_pointer(RtArena *, void *);\n"
    "\n"
    "/* Runtime format specifier functions */\n"
    "extern char *rt_format_long(RtArena *, long long, const char *);\n"
    "extern char *rt_format_double(RtArena *, double, const char *);\n"
    "extern char *rt_format_string(RtArena *, const char *, const char *);\n"
    "\n"
    "/* Handle-based string functions */\n"
    "extern RtHandle rt_str_concat_h(RtManagedArena *, RtHandle, const char *, const char *);\n"
    "extern RtHandle rt_str_append_h(RtManagedArena *, RtHandle, const char *, const char *);\n"
    "extern RtHandle rt_to_string_long_h(RtManagedArena *, long long);\n"
    "extern RtHandle rt_to_string_double_h(RtManagedArena *, double);\n"
    "extern RtHandle rt_to_string_char_h(RtManagedArena *, char);\n"
    "extern RtHandle rt_to_string_bool_h(RtManagedArena *, int);\n"
    "extern RtHandle rt_to_string_byte_h(RtManagedArena *, unsigned char);\n"
    "extern RtHandle rt_to_string_string_h(RtManagedArena *, const char *);\n"
    "extern RtHandle rt_str_substring_h(RtManagedArena *, const char *, long, long);\n"
    "extern RtHandle rt_str_toUpper_h(RtManagedArena *, const char *);\n"
    "extern RtHandle rt_str_toLower_h(RtManagedArena *, const char *);\n"
    "extern RtHandle rt_str_trim_h(RtManagedArena *, const char *);\n"
    "extern RtHandle rt_str_replace_h(RtManagedArena *, const char *, const char *, const char *);\n"
    "extern RtHandle rt_str_split_h(RtManagedArena *, const char *, const char *);\n"
    "extern RtHandle rt_format_long_h(RtManagedArena *, long long, const char *);\n"
    "extern RtHandle rt_format_double_h(RtManagedArena *, double, const char *);\n"
    "extern RtHandle rt_format_string_h(RtManagedArena *, const char *, const char *);\n"
    "\n"
    "/* Handle-based array functions */\n"
    "extern RtHandle rt_array_create_long_h(RtManagedArena *, size_t, const long long *);\n"
    "extern RtHandle rt_array_create_double_h(RtManagedArena *, size_t, const double *);\n"
    "extern RtHandle rt_array_create_char_h(RtManagedArena *, size_t, const char *);\n"
    "extern RtHandle rt_array_create_bool_h(RtManagedArena *, size_t, const int *);\n"
    "extern RtHandle rt_array_create_byte_h(RtManagedArena *, size_t, const unsigned char *);\n"
    "extern RtHandle rt_array_create_string_h(RtManagedArena *, size_t, const char **);\n"
    "extern RtHandle rt_array_create_ptr_h(RtManagedArena *, size_t, void **);\n"
    "extern RtHandle rt_array_push_long_h(RtManagedArena *, RtHandle, long long);\n"
    "extern RtHandle rt_array_push_double_h(RtManagedArena *, RtHandle, double);\n"
    "extern RtHandle rt_array_push_char_h(RtManagedArena *, RtHandle, char);\n"
    "extern RtHandle rt_array_push_bool_h(RtManagedArena *, RtHandle, int);\n"
    "extern RtHandle rt_array_push_byte_h(RtManagedArena *, RtHandle, unsigned char);\n"
    "extern RtHandle rt_array_push_string_h(RtManagedArena *, RtHandle, const char *);\n"
    "extern RtHandle rt_array_push_ptr_h(RtManagedArena *, RtHandle, void *);\n"
    "extern RtHandle rt_array_push_voidptr_h(RtManagedArena *, RtHandle, void *);\n"
    "extern RtHandle rt_array_push_any_h(RtManagedArena *, RtHandle, RtAny);\n"
    "extern long long rt_array_pop_long_h(RtManagedArena *, RtHandle);\n"
    "extern double rt_array_pop_double_h(RtManagedArena *, RtHandle);\n"
    "extern char rt_array_pop_char_h(RtManagedArena *, RtHandle);\n"
    "extern int rt_array_pop_bool_h(RtManagedArena *, RtHandle);\n"
    "extern unsigned char rt_array_pop_byte_h(RtManagedArena *, RtHandle);\n"
    "extern char *rt_array_pop_string_h(RtManagedArena *, RtHandle);\n"
    "extern void *rt_array_pop_ptr_h(RtManagedArena *, RtHandle);\n"
    "extern RtHandle rt_array_clone_long_h(RtManagedArena *, RtHandle, const long long *);\n"
    "extern RtHandle rt_array_clone_double_h(RtManagedArena *, RtHandle, const double *);\n"
    "extern RtHandle rt_array_clone_char_h(RtManagedArena *, RtHandle, const char *);\n"
    "extern RtHandle rt_array_clone_bool_h(RtManagedArena *, RtHandle, const int *);\n"
    "extern RtHandle rt_array_clone_byte_h(RtManagedArena *, RtHandle, const unsigned char *);\n"
    "extern RtHandle rt_array_clone_string_h(RtManagedArena *, RtHandle, const char **);\n"
    "extern RtHandle rt_array_clone_ptr_h(RtManagedArena *, RtHandle, void **);\n"
    "extern RtHandle rt_array_clone_int32_h(RtManagedArena *, RtHandle, const int32_t *);\n"
    "extern RtHandle rt_array_clone_uint32_h(RtManagedArena *, RtHandle, const uint32_t *);\n"
    "extern RtHandle rt_array_clone_uint_h(RtManagedArena *, RtHandle, const uint64_t *);\n"
    "extern RtHandle rt_array_clone_float_h(RtManagedArena *, RtHandle, const float *);\n"
    "extern RtHandle rt_array_clone_void_h(RtManagedArena *, RtHandle, const RtAny *);\n"
    "extern RtHandle rt_array_from_raw_strings_h(RtManagedArena *, RtHandle, const char **);\n"
    "extern int rt_array_eq_string_h(RtManagedArena *, RtHandle, RtHandle);\n"
    "extern char *rt_to_string_array_string_h(RtManagedArena *, RtHandle *);\n"
    "extern char *rt_to_string_array2_long_h(RtManagedArena *, RtHandle *);\n"
    "extern char *rt_to_string_array2_double_h(RtManagedArena *, RtHandle *);\n"
    "extern char *rt_to_string_array2_char_h(RtManagedArena *, RtHandle *);\n"
    "extern char *rt_to_string_array2_bool_h(RtManagedArena *, RtHandle *);\n"
    "extern char *rt_to_string_array2_byte_h(RtManagedArena *, RtHandle *);\n"
    "extern char *rt_to_string_array2_string_h(RtManagedArena *, RtHandle *);\n"
    "extern RtHandle rt_array_concat_long_h(RtManagedArena *, RtHandle, const long long *, const long long *);\n"
    "extern RtHandle rt_array_concat_double_h(RtManagedArena *, RtHandle, const double *, const double *);\n"
    "extern RtHandle rt_array_concat_char_h(RtManagedArena *, RtHandle, const char *, const char *);\n"
    "extern RtHandle rt_array_concat_bool_h(RtManagedArena *, RtHandle, const int *, const int *);\n"
    "extern RtHandle rt_array_concat_byte_h(RtManagedArena *, RtHandle, const unsigned char *, const unsigned char *);\n"
    "extern RtHandle rt_array_concat_string_h(RtManagedArena *, RtHandle, const char **, const char **);\n"
    "extern RtHandle rt_array_concat_ptr_h(RtManagedArena *, RtHandle, void **, void **);\n"
    "extern RtHandle rt_array_slice_long_h(RtManagedArena *, const long long *, long, long, long);\n"
    "extern RtHandle rt_array_slice_double_h(RtManagedArena *, const double *, long, long, long);\n"
    "extern RtHandle rt_array_slice_char_h(RtManagedArena *, const char *, long, long, long);\n"
    "extern RtHandle rt_array_slice_bool_h(RtManagedArena *, const int *, long, long, long);\n"
    "extern RtHandle rt_array_slice_byte_h(RtManagedArena *, const unsigned char *, long, long, long);\n"
    "extern RtHandle rt_array_slice_string_h(RtManagedArena *, const char **, long, long, long);\n"
    "extern RtHandle rt_array_slice_int32_h(RtManagedArena *, const int32_t *, long, long, long);\n"
    "extern RtHandle rt_array_slice_uint32_h(RtManagedArena *, const uint32_t *, long, long, long);\n"
    "extern RtHandle rt_array_slice_uint_h(RtManagedArena *, const uint64_t *, long, long, long);\n"
    "extern RtHandle rt_array_slice_float_h(RtManagedArena *, const float *, long, long, long);\n"
    "extern RtHandle rt_array_rev_long_h(RtManagedArena *, const long long *);\n"
    "extern RtHandle rt_array_rev_double_h(RtManagedArena *, const double *);\n"
    "extern RtHandle rt_array_rev_char_h(RtManagedArena *, const char *);\n"
    "extern RtHandle rt_array_rev_bool_h(RtManagedArena *, const int *);\n"
    "extern RtHandle rt_array_rev_byte_h(RtManagedArena *, const unsigned char *);\n"
    "extern RtHandle rt_array_rev_string_h(RtManagedArena *, const char **);\n"
    "extern RtHandle rt_array_rem_long_h(RtManagedArena *, const long long *, long);\n"
    "extern RtHandle rt_array_rem_double_h(RtManagedArena *, const double *, long);\n"
    "extern RtHandle rt_array_rem_char_h(RtManagedArena *, const char *, long);\n"
    "extern RtHandle rt_array_rem_bool_h(RtManagedArena *, const int *, long);\n"
    "extern RtHandle rt_array_rem_byte_h(RtManagedArena *, const unsigned char *, long);\n"
    "extern RtHandle rt_array_rem_string_h(RtManagedArena *, const char **, long);\n"
    "extern RtHandle rt_array_ins_long_h(RtManagedArena *, const long long *, long long, long);\n"
    "extern RtHandle rt_array_ins_double_h(RtManagedArena *, const double *, double, long);\n"
    "extern RtHandle rt_array_ins_char_h(RtManagedArena *, const char *, char, long);\n"
    "extern RtHandle rt_array_ins_bool_h(RtManagedArena *, const int *, int, long);\n"
    "extern RtHandle rt_array_ins_byte_h(RtManagedArena *, const unsigned char *, unsigned char, long);\n"
    "extern RtHandle rt_array_ins_string_h(RtManagedArena *, const char **, const char *, long);\n"
    "extern RtHandle rt_array_push_copy_long_h(RtManagedArena *, const long long *, long long);\n"
    "extern RtHandle rt_array_push_copy_double_h(RtManagedArena *, const double *, double);\n"
    "extern RtHandle rt_array_push_copy_char_h(RtManagedArena *, const char *, char);\n"
    "extern RtHandle rt_array_push_copy_bool_h(RtManagedArena *, const int *, int);\n"
    "extern RtHandle rt_array_push_copy_byte_h(RtManagedArena *, const unsigned char *, unsigned char);\n"
    "extern RtHandle rt_array_push_copy_string_h(RtManagedArena *, const char **, const char *);\n"
    "extern RtHandle rt_array_alloc_long_h(RtManagedArena *, size_t, long long);\n"
    "extern RtHandle rt_array_alloc_double_h(RtManagedArena *, size_t, double);\n"
    "extern RtHandle rt_array_alloc_char_h(RtManagedArena *, size_t, char);\n"
    "extern RtHandle rt_array_alloc_bool_h(RtManagedArena *, size_t, int);\n"
    "extern RtHandle rt_array_alloc_byte_h(RtManagedArena *, size_t, unsigned char);\n"
    "extern RtHandle rt_array_alloc_string_h(RtManagedArena *, size_t, const char *);\n"
    "extern RtHandle rt_array_range_h(RtManagedArena *, long long, long long);\n"
    "extern RtHandle rt_args_create_h(RtManagedArena *, int, char **);\n"
    "\n"
    "/* Runtime long arithmetic (comparisons are static inline in runtime.h) */\n"
    "extern long long rt_add_long(long long, long long);\n"
    "extern long long rt_sub_long(long long, long long);\n"
    "extern long long rt_mul_long(long long, long long);\n"
    "extern long long rt_div_long(long long, long long);\n"
    "extern long long rt_mod_long(long long, long long);\n"
    "extern long long rt_neg_long(long long);\n"
    "extern long long rt_post_inc_long(long long *);\n"
    "extern long long rt_post_dec_long(long long *);\n"
    "\n"
    "/* Runtime double arithmetic (comparisons are static inline in runtime.h) */\n"
    "extern double rt_add_double(double, double);\n"
    "extern double rt_sub_double(double, double);\n"
    "extern double rt_mul_double(double, double);\n"
    "extern double rt_div_double(double, double);\n"
    "extern double rt_neg_double(double);\n"
    "\n"
    "/* Runtime array operations */\n"
    "extern long long *rt_array_push_long(RtArena *, long long *, long long);\n"
    "extern double *rt_array_push_double(RtArena *, double *, double);\n"
    "extern char *rt_array_push_char(RtArena *, char *, char);\n"
    "extern char **rt_array_push_string(RtArena *, char **, const char *);\n"
    "extern int *rt_array_push_bool(RtArena *, int *, int);\n"
    "extern unsigned char *rt_array_push_byte(RtArena *, unsigned char *, unsigned char);\n"
    "extern void **rt_array_push_ptr(RtArena *, void **, void *);\n"
    "\n"
    "/* Runtime array print functions */\n"
    "extern void rt_print_array_long(long long *);\n"
    "extern void rt_print_array_double(double *);\n"
    "extern void rt_print_array_char(char *);\n"
    "extern void rt_print_array_bool(int *);\n"
    "extern void rt_print_array_byte(unsigned char *);\n"
    "extern void rt_print_array_string(char **);\n"
    "extern void rt_print_array_string_h(RtManagedArena *, RtHandle *);\n"
    "\n"
    "/* Runtime array clear */\n"
    "extern void rt_array_clear(void *);\n"
    "\n"
    "/* Runtime array pop functions */\n"
    "extern long long rt_array_pop_long(long long *);\n"
    "extern double rt_array_pop_double(double *);\n"
    "extern char rt_array_pop_char(char *);\n"
    "extern int rt_array_pop_bool(int *);\n"
    "extern unsigned char rt_array_pop_byte(unsigned char *);\n"
    "extern char *rt_array_pop_string(char **);\n"
    "extern void *rt_array_pop_ptr(void **);\n"
    "\n"
    "/* Runtime array concat functions */\n"
    "extern long long *rt_array_concat_long(RtArena *, long long *, long long *);\n"
    "extern double *rt_array_concat_double(RtArena *, double *, double *);\n"
    "extern char *rt_array_concat_char(RtArena *, char *, char *);\n"
    "extern int *rt_array_concat_bool(RtArena *, int *, int *);\n"
    "extern unsigned char *rt_array_concat_byte(RtArena *, unsigned char *, unsigned char *);\n"
    "extern char **rt_array_concat_string(RtArena *, char **, char **);\n"
    "extern void **rt_array_concat_ptr(RtArena *, void **, void **);\n"
    "\n"
    "/* Runtime array slice functions (start, end, step) */\n"
    "extern long long *rt_array_slice_long(RtArena *, long long *, long, long, long);\n"
    "extern double *rt_array_slice_double(RtArena *, double *, long, long, long);\n"
    "extern char *rt_array_slice_char(RtArena *, char *, long, long, long);\n"
    "extern int *rt_array_slice_bool(RtArena *, int *, long, long, long);\n"
    "extern unsigned char *rt_array_slice_byte(RtArena *, unsigned char *, long, long, long);\n"
    "extern char **rt_array_slice_string(RtArena *, char **, long, long, long);\n"
    "\n"
    "/* Runtime array reverse functions */\n"
    "extern long long *rt_array_rev_long(RtArena *, long long *);\n"
    "extern double *rt_array_rev_double(RtArena *, double *);\n"
    "extern char *rt_array_rev_char(RtArena *, char *);\n"
    "extern int *rt_array_rev_bool(RtArena *, int *);\n"
    "extern unsigned char *rt_array_rev_byte(RtArena *, unsigned char *);\n"
    "extern char **rt_array_rev_string(RtArena *, char **);\n"
    "\n"
    "/* Runtime array remove functions */\n"
    "extern long long *rt_array_rem_long(RtArena *, long long *, long);\n"
    "extern double *rt_array_rem_double(RtArena *, double *, long);\n"
    "extern char *rt_array_rem_char(RtArena *, char *, long);\n"
    "extern int *rt_array_rem_bool(RtArena *, int *, long);\n"
    "extern unsigned char *rt_array_rem_byte(RtArena *, unsigned char *, long);\n"
    "extern char **rt_array_rem_string(RtArena *, char **, long);\n"
    "\n"
    "/* Runtime array insert functions */\n"
    "extern long long *rt_array_ins_long(RtArena *, long long *, long long, long);\n"
    "extern double *rt_array_ins_double(RtArena *, double *, double, long);\n"
    "extern char *rt_array_ins_char(RtArena *, char *, char, long);\n"
    "extern int *rt_array_ins_bool(RtArena *, int *, int, long);\n"
    "extern unsigned char *rt_array_ins_byte(RtArena *, unsigned char *, unsigned char, long);\n"
    "extern char **rt_array_ins_string(RtArena *, char **, const char *, long);\n"
    "\n"
    "/* Runtime array push (copy) functions */\n"
    "extern long long *rt_array_push_copy_long(RtArena *, long long *, long long);\n"
    "extern double *rt_array_push_copy_double(RtArena *, double *, double);\n"
    "extern char *rt_array_push_copy_char(RtArena *, char *, char);\n"
    "extern int *rt_array_push_copy_bool(RtArena *, int *, int);\n"
    "extern unsigned char *rt_array_push_copy_byte(RtArena *, unsigned char *, unsigned char);\n"
    "extern char **rt_array_push_copy_string(RtArena *, char **, const char *);\n"
    "\n"
    "/* Runtime array indexOf functions */\n"
    "extern long rt_array_indexOf_long(long long *, long long);\n"
    "extern long rt_array_indexOf_double(double *, double);\n"
    "extern long rt_array_indexOf_char(char *, char);\n"
    "extern long rt_array_indexOf_bool(int *, int);\n"
    "extern long rt_array_indexOf_byte(unsigned char *, unsigned char);\n"
    "extern long rt_array_indexOf_string(char **, const char *);\n"
    "extern long rt_array_indexOf_string_h(RtManagedArena *, RtHandle *, const char *);\n"
    "\n"
    "/* Runtime array contains functions */\n"
    "extern int rt_array_contains_long(long long *, long long);\n"
    "extern int rt_array_contains_double(double *, double);\n"
    "extern int rt_array_contains_char(char *, char);\n"
    "extern int rt_array_contains_bool(int *, int);\n"
    "extern int rt_array_contains_byte(unsigned char *, unsigned char);\n"
    "extern int rt_array_contains_string(char **, const char *);\n"
    "extern int rt_array_contains_string_h(RtManagedArena *, RtHandle *, const char *);\n"
    "\n"
    "/* Runtime array clone functions */\n"
    "extern long long *rt_array_clone_long(RtArena *, long long *);\n"
    "extern double *rt_array_clone_double(RtArena *, double *);\n"
    "extern char *rt_array_clone_char(RtArena *, char *);\n"
    "extern int *rt_array_clone_bool(RtArena *, int *);\n"
    "extern unsigned char *rt_array_clone_byte(RtArena *, unsigned char *);\n"
    "extern char **rt_array_clone_string(RtArena *, char **);\n"
    "\n"
    "/* Runtime array join functions */\n"
    "extern char *rt_array_join_long(RtArena *, long long *, const char *);\n"
    "extern char *rt_array_join_double(RtArena *, double *, const char *);\n"
    "extern char *rt_array_join_char(RtArena *, char *, const char *);\n"
    "extern char *rt_array_join_bool(RtArena *, int *, const char *);\n"
    "extern char *rt_array_join_byte(RtArena *, unsigned char *, const char *);\n"
    "extern char *rt_array_join_string(RtArena *, char **, const char *);\n"
    "extern char *rt_array_join_string_h(RtManagedArena *, RtHandle *, const char *);\n"
    "\n"
    "/* Runtime array create from static data */\n"
    "extern long long *rt_array_create_long(RtArena *, size_t, const long long *);\n"
    "extern double *rt_array_create_double(RtArena *, size_t, const double *);\n"
    "extern char *rt_array_create_char(RtArena *, size_t, const char *);\n"
    "extern int *rt_array_create_bool(RtArena *, size_t, const int *);\n"
    "extern unsigned char *rt_array_create_byte(RtArena *, size_t, const unsigned char *);\n"
    "extern char **rt_array_create_string(RtArena *, size_t, const char **);\n"
    "\n"
    "/* Runtime array equality functions */\n"
    "extern int rt_array_eq_long(long long *, long long *);\n"
    "extern int rt_array_eq_double(double *, double *);\n"
    "extern int rt_array_eq_char(char *, char *);\n"
    "extern int rt_array_eq_bool(int *, int *);\n"
    "extern int rt_array_eq_byte(unsigned char *, unsigned char *);\n"
    "extern int rt_array_eq_string(char **, char **);\n"
    "\n"
    "/* Runtime range creation */\n"
    "extern long long *rt_array_range(RtArena *, long long, long long);\n"
    "\n"
    "/* Global convenience functions (readLine, println, printErr, printErrLn) */\n"
    "extern char *rt_read_line(RtArena *);\n"
    "extern void rt_println(const char *);\n"
    "extern void rt_print_err(const char *);\n"
    "extern void rt_print_err_ln(const char *);\n"
    "\n"
    "/* Byte array extension methods */\n"
    "extern char *rt_byte_array_to_string(RtArena *, unsigned char *);\n"
    "extern char *rt_byte_array_to_string_latin1(RtArena *, unsigned char *);\n"
    "extern char *rt_byte_array_to_hex(RtArena *, unsigned char *);\n"
    "extern char *rt_byte_array_to_base64(RtArena *, unsigned char *);\n"
    "extern unsigned char *rt_string_to_bytes(RtArena *, const char *);\n"
    "\n"
    "/* String splitting methods */\n"
    "extern char **rt_str_split_whitespace(RtArena *, const char *);\n"
    "extern char **rt_str_split_lines(RtArena *, const char *);\n"
    "extern int rt_str_is_blank(const char *);\n"
    "\n"
    "/* Mutable string operations */\n"
    "extern char *rt_string_with_capacity(RtArena *, size_t);\n"
    "extern char *rt_string_from(RtArena *, const char *);\n"
    "extern char *rt_string_ensure_mutable(RtArena *, char *);\n"
    "extern char *rt_string_append(char *, const char *);\n"
    "\n"
    "/* Environment operations */\n"
    "extern char *rt_env_get(RtArena *, const char *);\n"
    "extern char **rt_env_names(RtArena *);\n"
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
