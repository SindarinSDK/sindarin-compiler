#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
 * Matches the output of code_gen_externs().
 */
static const char *CODE_GEN_RUNTIME_HEADER =
    "#include <stdlib.h>\n"
    "#include <string.h>\n"
    "#include <stdio.h>\n"
    "#include <stdbool.h>\n"
    "#include <stdint.h>\n"  /* For int32_t, uint32_t, uint64_t */
    "#include <limits.h>\n"
    "#include <setjmp.h>\n"  /* For thread panic handling */
    /* pthread.h is now included via runtime.h -> runtime_thread.h (handles platform compatibility) */
    "#include \"runtime.h\"\n"
    "#ifdef _WIN32\n"
    "#undef min\n"
    "#undef max\n"
    "#endif\n\n"
    "/* Runtime arena operations */\n"
    "typedef struct RtArena RtArena;\n"
    "extern RtArena *rt_arena_create(RtArena *parent);\n"
    "extern void rt_arena_destroy(RtArena *arena);\n"
    "extern void *rt_arena_alloc(RtArena *arena, size_t size);\n\n"
    "/* Closure type for lambdas */\n"
    "typedef struct __Closure__ { void *fn; RtArena *arena; size_t size; } __Closure__;\n\n"
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
    "extern long rt_str_charAt(const char *, long);\n\n"
    "/* Runtime print functions */\n"
    "extern void rt_print_long(long long);\n"
    "extern void rt_print_double(double);\n"
    "extern void rt_print_char(long);\n"
    "extern void rt_print_string(const char *);\n"
    "extern void rt_print_bool(long);\n"
    "extern void rt_print_byte(unsigned char);\n\n"
    "/* Runtime type conversions */\n"
    "extern char *rt_to_string_long(RtArena *, long long);\n"
    "extern char *rt_to_string_double(RtArena *, double);\n"
    "extern char *rt_to_string_char(RtArena *, char);\n"
    "extern char *rt_to_string_bool(RtArena *, int);\n"
    "extern char *rt_to_string_byte(RtArena *, unsigned char);\n"
    "extern char *rt_to_string_string(RtArena *, const char *);\n"
    "extern char *rt_to_string_void(RtArena *);\n"
    "extern char *rt_to_string_pointer(RtArena *, void *);\n\n"
    "/* Runtime format specifier functions */\n"
    "extern char *rt_format_long(RtArena *, long long, const char *);\n"
    "extern char *rt_format_double(RtArena *, double, const char *);\n"
    "extern char *rt_format_string(RtArena *, const char *, const char *);\n\n"
    "/* Runtime long arithmetic (comparisons are static inline in runtime.h) */\n"
    "extern long long rt_add_long(long long, long long);\n"
    "extern long long rt_sub_long(long long, long long);\n"
    "extern long long rt_mul_long(long long, long long);\n"
    "extern long long rt_div_long(long long, long long);\n"
    "extern long long rt_mod_long(long long, long long);\n"
    "extern long long rt_neg_long(long long);\n"
    "extern long long rt_post_inc_long(long long *);\n"
    "extern long long rt_post_dec_long(long long *);\n\n"
    "/* Runtime double arithmetic (comparisons are static inline in runtime.h) */\n"
    "extern double rt_add_double(double, double);\n"
    "extern double rt_sub_double(double, double);\n"
    "extern double rt_mul_double(double, double);\n"
    "extern double rt_div_double(double, double);\n"
    "extern double rt_neg_double(double);\n\n"
    "/* Runtime array operations */\n"
    "extern long long *rt_array_push_long(RtArena *, long long *, long long);\n"
    "extern double *rt_array_push_double(RtArena *, double *, double);\n"
    "extern char *rt_array_push_char(RtArena *, char *, char);\n"
    "extern char **rt_array_push_string(RtArena *, char **, const char *);\n"
    "extern int *rt_array_push_bool(RtArena *, int *, int);\n"
    "extern unsigned char *rt_array_push_byte(RtArena *, unsigned char *, unsigned char);\n"
    "extern void **rt_array_push_ptr(RtArena *, void **, void *);\n\n"
    "/* Runtime array print functions */\n"
    "extern void rt_print_array_long(long long *);\n"
    "extern void rt_print_array_double(double *);\n"
    "extern void rt_print_array_char(char *);\n"
    "extern void rt_print_array_bool(int *);\n"
    "extern void rt_print_array_byte(unsigned char *);\n"
    "extern void rt_print_array_string(char **);\n\n"
    "/* Runtime array clear */\n"
    "extern void rt_array_clear(void *);\n\n"
    "/* Runtime array pop functions */\n"
    "extern long long rt_array_pop_long(long long *);\n"
    "extern double rt_array_pop_double(double *);\n"
    "extern char rt_array_pop_char(char *);\n"
    "extern int rt_array_pop_bool(int *);\n"
    "extern unsigned char rt_array_pop_byte(unsigned char *);\n"
    "extern char *rt_array_pop_string(char **);\n"
    "extern void *rt_array_pop_ptr(void **);\n\n"
    "/* Runtime array concat functions */\n"
    "extern long long *rt_array_concat_long(RtArena *, long long *, long long *);\n"
    "extern double *rt_array_concat_double(RtArena *, double *, double *);\n"
    "extern char *rt_array_concat_char(RtArena *, char *, char *);\n"
    "extern int *rt_array_concat_bool(RtArena *, int *, int *);\n"
    "extern unsigned char *rt_array_concat_byte(RtArena *, unsigned char *, unsigned char *);\n"
    "extern char **rt_array_concat_string(RtArena *, char **, char **);\n"
    "extern void **rt_array_concat_ptr(RtArena *, void **, void **);\n\n"
    "/* Runtime array slice functions (start, end, step) */\n"
    "extern long long *rt_array_slice_long(RtArena *, long long *, long, long, long);\n"
    "extern double *rt_array_slice_double(RtArena *, double *, long, long, long);\n"
    "extern char *rt_array_slice_char(RtArena *, char *, long, long, long);\n"
    "extern int *rt_array_slice_bool(RtArena *, int *, long, long, long);\n"
    "extern unsigned char *rt_array_slice_byte(RtArena *, unsigned char *, long, long, long);\n"
    "extern char **rt_array_slice_string(RtArena *, char **, long, long, long);\n\n"
    "/* Runtime array reverse functions */\n"
    "extern long long *rt_array_rev_long(RtArena *, long long *);\n"
    "extern double *rt_array_rev_double(RtArena *, double *);\n"
    "extern char *rt_array_rev_char(RtArena *, char *);\n"
    "extern int *rt_array_rev_bool(RtArena *, int *);\n"
    "extern unsigned char *rt_array_rev_byte(RtArena *, unsigned char *);\n"
    "extern char **rt_array_rev_string(RtArena *, char **);\n\n"
    "/* Runtime array remove functions */\n"
    "extern long long *rt_array_rem_long(RtArena *, long long *, long);\n"
    "extern double *rt_array_rem_double(RtArena *, double *, long);\n"
    "extern char *rt_array_rem_char(RtArena *, char *, long);\n"
    "extern int *rt_array_rem_bool(RtArena *, int *, long);\n"
    "extern unsigned char *rt_array_rem_byte(RtArena *, unsigned char *, long);\n"
    "extern char **rt_array_rem_string(RtArena *, char **, long);\n\n"
    "/* Runtime array insert functions */\n"
    "extern long long *rt_array_ins_long(RtArena *, long long *, long long, long);\n"
    "extern double *rt_array_ins_double(RtArena *, double *, double, long);\n"
    "extern char *rt_array_ins_char(RtArena *, char *, char, long);\n"
    "extern int *rt_array_ins_bool(RtArena *, int *, int, long);\n"
    "extern unsigned char *rt_array_ins_byte(RtArena *, unsigned char *, unsigned char, long);\n"
    "extern char **rt_array_ins_string(RtArena *, char **, const char *, long);\n\n"
    "/* Runtime array push (copy) functions */\n"
    "extern long long *rt_array_push_copy_long(RtArena *, long long *, long long);\n"
    "extern double *rt_array_push_copy_double(RtArena *, double *, double);\n"
    "extern char *rt_array_push_copy_char(RtArena *, char *, char);\n"
    "extern int *rt_array_push_copy_bool(RtArena *, int *, int);\n"
    "extern unsigned char *rt_array_push_copy_byte(RtArena *, unsigned char *, unsigned char);\n"
    "extern char **rt_array_push_copy_string(RtArena *, char **, const char *);\n\n"
    "/* Runtime array indexOf functions */\n"
    "extern long rt_array_indexOf_long(long long *, long long);\n"
    "extern long rt_array_indexOf_double(double *, double);\n"
    "extern long rt_array_indexOf_char(char *, char);\n"
    "extern long rt_array_indexOf_bool(int *, int);\n"
    "extern long rt_array_indexOf_byte(unsigned char *, unsigned char);\n"
    "extern long rt_array_indexOf_string(char **, const char *);\n\n"
    "/* Runtime array contains functions */\n"
    "extern int rt_array_contains_long(long long *, long long);\n"
    "extern int rt_array_contains_double(double *, double);\n"
    "extern int rt_array_contains_char(char *, char);\n"
    "extern int rt_array_contains_bool(int *, int);\n"
    "extern int rt_array_contains_byte(unsigned char *, unsigned char);\n"
    "extern int rt_array_contains_string(char **, const char *);\n\n"
    "/* Runtime array clone functions */\n"
    "extern long long *rt_array_clone_long(RtArena *, long long *);\n"
    "extern double *rt_array_clone_double(RtArena *, double *);\n"
    "extern char *rt_array_clone_char(RtArena *, char *);\n"
    "extern int *rt_array_clone_bool(RtArena *, int *);\n"
    "extern unsigned char *rt_array_clone_byte(RtArena *, unsigned char *);\n"
    "extern char **rt_array_clone_string(RtArena *, char **);\n\n"
    "/* Runtime array join functions */\n"
    "extern char *rt_array_join_long(RtArena *, long long *, const char *);\n"
    "extern char *rt_array_join_double(RtArena *, double *, const char *);\n"
    "extern char *rt_array_join_char(RtArena *, char *, const char *);\n"
    "extern char *rt_array_join_bool(RtArena *, int *, const char *);\n"
    "extern char *rt_array_join_byte(RtArena *, unsigned char *, const char *);\n"
    "extern char *rt_array_join_string(RtArena *, char **, const char *);\n\n"
    "/* Runtime array create from static data */\n"
    "extern long long *rt_array_create_long(RtArena *, size_t, const long long *);\n"
    "extern double *rt_array_create_double(RtArena *, size_t, const double *);\n"
    "extern char *rt_array_create_char(RtArena *, size_t, const char *);\n"
    "extern int *rt_array_create_bool(RtArena *, size_t, const int *);\n"
    "extern unsigned char *rt_array_create_byte(RtArena *, size_t, const unsigned char *);\n"
    "extern char **rt_array_create_string(RtArena *, size_t, const char **);\n\n"
    "/* Runtime array equality functions */\n"
    "extern int rt_array_eq_long(long long *, long long *);\n"
    "extern int rt_array_eq_double(double *, double *);\n"
    "extern int rt_array_eq_char(char *, char *);\n"
    "extern int rt_array_eq_bool(int *, int *);\n"
    "extern int rt_array_eq_byte(unsigned char *, unsigned char *);\n"
    "extern int rt_array_eq_string(char **, char **);\n\n"
    "/* Runtime range creation */\n"
    "extern long long *rt_array_range(RtArena *, long long, long long);\n\n"
    "/* TextFile static methods */\n"
    "typedef struct RtTextFile RtTextFile;\n"
    "extern RtTextFile *rt_text_file_open(RtArena *, const char *);\n"
    "extern int rt_text_file_exists(const char *);\n"
    "extern char *rt_text_file_read_all(RtArena *, const char *);\n"
    "extern void rt_text_file_write_all(const char *, const char *);\n"
    "extern void rt_text_file_delete(const char *);\n"
    "extern void rt_text_file_copy(const char *, const char *);\n"
    "extern void rt_text_file_move(const char *, const char *);\n"
    "extern void rt_text_file_close(RtTextFile *);\n\n"
    "/* TextFile instance reading methods */\n"
    "extern long rt_text_file_read_char(RtTextFile *);\n"
    "extern char *rt_text_file_read_word(RtArena *, RtTextFile *);\n"
    "extern char *rt_text_file_read_line(RtArena *, RtTextFile *);\n"
    "extern char *rt_text_file_instance_read_all(RtArena *, RtTextFile *);\n"
    "extern char **rt_text_file_read_lines(RtArena *, RtTextFile *);\n"
    "extern long rt_text_file_read_into(RtTextFile *, char *);\n\n"
    "/* TextFile instance writing methods */\n"
    "extern void rt_text_file_write_char(RtTextFile *, long);\n"
    "extern void rt_text_file_write(RtTextFile *, const char *);\n"
    "extern void rt_text_file_write_line(RtTextFile *, const char *);\n"
    "extern void rt_text_file_print(RtTextFile *, const char *);\n"
    "extern void rt_text_file_println(RtTextFile *, const char *);\n\n"
    "/* TextFile state methods */\n"
    "extern int rt_text_file_has_chars(RtTextFile *);\n"
    "extern int rt_text_file_has_words(RtTextFile *);\n"
    "extern int rt_text_file_has_lines(RtTextFile *);\n"
    "extern int rt_text_file_is_eof(RtTextFile *);\n"
    "extern long rt_text_file_position(RtTextFile *);\n"
    "extern void rt_text_file_seek(RtTextFile *, long);\n"
    "extern void rt_text_file_rewind(RtTextFile *);\n"
    "extern void rt_text_file_flush(RtTextFile *);\n\n"
    "/* TextFile properties */\n"
    "extern char *rt_text_file_get_path(RtArena *, RtTextFile *);\n"
    "extern char *rt_text_file_get_name(RtArena *, RtTextFile *);\n"
    "extern long rt_text_file_get_size(RtTextFile *);\n\n"
    "/* BinaryFile static methods */\n"
    "typedef struct RtBinaryFile RtBinaryFile;\n"
    "extern RtBinaryFile *rt_binary_file_open(RtArena *, const char *);\n"
    "extern int rt_binary_file_exists(const char *);\n"
    "extern unsigned char *rt_binary_file_read_all(RtArena *, const char *);\n"
    "extern void rt_binary_file_write_all(const char *, unsigned char *);\n"
    "extern void rt_binary_file_delete(const char *);\n"
    "extern void rt_binary_file_copy(const char *, const char *);\n"
    "extern void rt_binary_file_move(const char *, const char *);\n"
    "extern void rt_binary_file_close(RtBinaryFile *);\n\n"
    "/* BinaryFile instance reading methods */\n"
    "extern long rt_binary_file_read_byte(RtBinaryFile *);\n"
    "extern unsigned char *rt_binary_file_read_bytes(RtArena *, RtBinaryFile *, long);\n"
    "extern unsigned char *rt_binary_file_instance_read_all(RtArena *, RtBinaryFile *);\n"
    "extern long rt_binary_file_read_into(RtBinaryFile *, unsigned char *);\n\n"
    "/* BinaryFile instance writing methods */\n"
    "extern void rt_binary_file_write_byte(RtBinaryFile *, long);\n"
    "extern void rt_binary_file_write_bytes(RtBinaryFile *, unsigned char *);\n\n"
    "/* BinaryFile state methods */\n"
    "extern int rt_binary_file_has_bytes(RtBinaryFile *);\n"
    "extern int rt_binary_file_is_eof(RtBinaryFile *);\n"
    "extern long rt_binary_file_position(RtBinaryFile *);\n"
    "extern void rt_binary_file_seek(RtBinaryFile *, long);\n"
    "extern void rt_binary_file_rewind(RtBinaryFile *);\n"
    "extern void rt_binary_file_flush(RtBinaryFile *);\n\n"
    "/* BinaryFile properties */\n"
    "extern char *rt_binary_file_get_path(RtArena *, RtBinaryFile *);\n"
    "extern char *rt_binary_file_get_name(RtArena *, RtBinaryFile *);\n"
    "extern long rt_binary_file_get_size(RtBinaryFile *);\n\n"
    "/* Standard streams (Stdin, Stdout, Stderr) */\n"
    "extern char *rt_stdin_read_line(RtArena *);\n"
    "extern long rt_stdin_read_char(void);\n"
    "extern char *rt_stdin_read_word(RtArena *);\n"
    "extern int rt_stdin_has_chars(void);\n"
    "extern int rt_stdin_has_lines(void);\n"
    "extern int rt_stdin_is_eof(void);\n"
    "extern void rt_stdout_write(const char *);\n"
    "extern void rt_stdout_write_line(const char *);\n"
    "extern void rt_stdout_flush(void);\n"
    "extern void rt_stderr_write(const char *);\n"
    "extern void rt_stderr_write_line(const char *);\n"
    "extern void rt_stderr_flush(void);\n\n"
    "/* Global convenience functions */\n"
    "extern char *rt_read_line(RtArena *);\n"
    "extern void rt_println(const char *);\n"
    "extern void rt_print_err(const char *);\n"
    "extern void rt_print_err_ln(const char *);\n\n"
    "/* Byte array extension methods */\n"
    "extern char *rt_byte_array_to_string(RtArena *, unsigned char *);\n"
    "extern char *rt_byte_array_to_string_latin1(RtArena *, unsigned char *);\n"
    "extern char *rt_byte_array_to_hex(RtArena *, unsigned char *);\n"
    "extern char *rt_byte_array_to_base64(RtArena *, unsigned char *);\n"
    "extern unsigned char *rt_string_to_bytes(RtArena *, const char *);\n"
    "extern unsigned char *rt_bytes_from_hex(RtArena *, const char *);\n"
    "extern unsigned char *rt_bytes_from_base64(RtArena *, const char *);\n\n"
    "/* Path utilities */\n"
    "extern char *rt_path_directory(RtArena *, const char *);\n"
    "extern char *rt_path_filename(RtArena *, const char *);\n"
    "extern char *rt_path_extension(RtArena *, const char *);\n"
    "extern char *rt_path_join2(RtArena *, const char *, const char *);\n"
    "extern char *rt_path_join3(RtArena *, const char *, const char *, const char *);\n"
    "extern char *rt_path_absolute(RtArena *, const char *);\n"
    "extern int rt_path_exists(const char *);\n"
    "extern int rt_path_is_file(const char *);\n"
    "extern int rt_path_is_directory(const char *);\n\n"
    "/* Directory operations */\n"
    "extern char **rt_directory_list(RtArena *, const char *);\n"
    "extern char **rt_directory_list_recursive(RtArena *, const char *);\n"
    "extern void rt_directory_create(const char *);\n"
    "extern void rt_directory_delete(const char *);\n"
    "extern void rt_directory_delete_recursive(const char *);\n\n"
    "/* String splitting methods */\n"
    "extern char **rt_str_split_whitespace(RtArena *, const char *);\n"
    "extern char **rt_str_split_lines(RtArena *, const char *);\n"
    "extern int rt_str_is_blank(const char *);\n\n"
    "/* Mutable string operations */\n"
    "extern char *rt_string_with_capacity(RtArena *, size_t);\n"
    "extern char *rt_string_from(RtArena *, const char *);\n"
    "extern char *rt_string_ensure_mutable(RtArena *, char *);\n"
    "extern char *rt_string_append(char *, const char *);\n\n"
    "/* Time type and operations */\n"
    "typedef struct RtTime RtTime;\n"
    "extern RtTime *rt_time_now(RtArena *);\n"
    "extern RtTime *rt_time_utc(RtArena *);\n"
    "extern RtTime *rt_time_from_millis(RtArena *, long long);\n"
    "extern RtTime *rt_time_from_seconds(RtArena *, long long);\n"
    "extern void rt_time_sleep(long);\n"
    "extern long long rt_time_get_millis(RtTime *);\n"
    "extern long long rt_time_get_seconds(RtTime *);\n"
    "extern long rt_time_get_year(RtTime *);\n"
    "extern long rt_time_get_month(RtTime *);\n"
    "extern long rt_time_get_day(RtTime *);\n"
    "extern long rt_time_get_hour(RtTime *);\n"
    "extern long rt_time_get_minute(RtTime *);\n"
    "extern long rt_time_get_second(RtTime *);\n"
    "extern long rt_time_get_weekday(RtTime *);\n"
    "extern char *rt_time_format(RtArena *, RtTime *, const char *);\n"
    "extern char *rt_time_to_iso(RtArena *, RtTime *);\n"
    "extern char *rt_time_to_date(RtArena *, RtTime *);\n"
    "extern char *rt_time_to_time(RtArena *, RtTime *);\n"
    "extern RtTime *rt_time_add(RtArena *, RtTime *, long long);\n"
    "extern RtTime *rt_time_add_seconds(RtArena *, RtTime *, long);\n"
    "extern RtTime *rt_time_add_minutes(RtArena *, RtTime *, long);\n"
    "extern RtTime *rt_time_add_hours(RtArena *, RtTime *, long);\n"
    "extern RtTime *rt_time_add_days(RtArena *, RtTime *, long);\n"
    "extern long long rt_time_diff(RtTime *, RtTime *);\n"
    "extern int rt_time_is_before(RtTime *, RtTime *);\n"
    "extern int rt_time_is_after(RtTime *, RtTime *);\n"
    "extern int rt_time_equals(RtTime *, RtTime *);\n\n"
    "/* Environment operations */\n"
    "extern char *rt_env_get(RtArena *, const char *);\n"
    "extern char **rt_env_names(RtArena *);\n\n"
    "/* Forward declarations */\n";

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
