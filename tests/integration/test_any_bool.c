#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <setjmp.h>
#include "runtime.h"
#ifdef _WIN32
#undef min
#undef max
#endif

/* Runtime arena operations */
typedef struct RtArena RtArena;
extern RtArena *rt_arena_create(RtArena *parent);
extern void rt_arena_destroy(RtArena *arena);
extern void *rt_arena_alloc(RtArena *arena, size_t size);

/* Closure type for lambdas */
typedef struct __Closure__ { void *fn; RtArena *arena; size_t size; } __Closure__;

/* Runtime string operations */
extern char *rt_str_concat(RtArena *, const char *, const char *);
extern long rt_str_length(const char *);
extern char *rt_str_substring(RtArena *, const char *, long, long);
extern long rt_str_indexOf(const char *, const char *);
extern char **rt_str_split(RtArena *, const char *, const char *);
extern char *rt_str_trim(RtArena *, const char *);
extern char *rt_str_toUpper(RtArena *, const char *);
extern char *rt_str_toLower(RtArena *, const char *);
extern int rt_str_startsWith(const char *, const char *);
extern int rt_str_endsWith(const char *, const char *);
extern int rt_str_contains(const char *, const char *);
extern char *rt_str_replace(RtArena *, const char *, const char *, const char *);
extern long rt_str_charAt(const char *, long);

/* Runtime print functions */
extern void rt_print_long(long long);
extern void rt_print_double(double);
extern void rt_print_char(long);
extern void rt_print_string(const char *);
extern void rt_print_bool(long);
extern void rt_print_byte(unsigned char);

/* Runtime type conversions */
extern char *rt_to_string_long(RtArena *, long long);
extern char *rt_to_string_double(RtArena *, double);
extern char *rt_to_string_char(RtArena *, char);
extern char *rt_to_string_bool(RtArena *, int);
extern char *rt_to_string_byte(RtArena *, unsigned char);
extern char *rt_to_string_string(RtArena *, const char *);
extern char *rt_to_string_void(RtArena *);
extern char *rt_to_string_pointer(RtArena *, void *);

/* Runtime format specifier functions */
extern char *rt_format_long(RtArena *, long long, const char *);
extern char *rt_format_double(RtArena *, double, const char *);
extern char *rt_format_string(RtArena *, const char *, const char *);

/* Runtime long arithmetic (comparisons are static inline in runtime.h) */
extern long long rt_add_long(long long, long long);
extern long long rt_sub_long(long long, long long);
extern long long rt_mul_long(long long, long long);
extern long long rt_div_long(long long, long long);
extern long long rt_mod_long(long long, long long);
extern long long rt_neg_long(long long);
extern long long rt_post_inc_long(long long *);
extern long long rt_post_dec_long(long long *);

/* Runtime double arithmetic (comparisons are static inline in runtime.h) */
extern double rt_add_double(double, double);
extern double rt_sub_double(double, double);
extern double rt_mul_double(double, double);
extern double rt_div_double(double, double);
extern double rt_neg_double(double);

/* Runtime array operations */
extern long long *rt_array_push_long(RtArena *, long long *, long long);
extern double *rt_array_push_double(RtArena *, double *, double);
extern char *rt_array_push_char(RtArena *, char *, char);
extern char **rt_array_push_string(RtArena *, char **, const char *);
extern int *rt_array_push_bool(RtArena *, int *, int);
extern unsigned char *rt_array_push_byte(RtArena *, unsigned char *, unsigned char);
extern void **rt_array_push_ptr(RtArena *, void **, void *);

/* Runtime array print functions */
extern void rt_print_array_long(long long *);
extern void rt_print_array_double(double *);
extern void rt_print_array_char(char *);
extern void rt_print_array_bool(int *);
extern void rt_print_array_byte(unsigned char *);
extern void rt_print_array_string(char **);

/* Runtime array clear */
extern void rt_array_clear(void *);

/* Runtime array pop functions */
extern long long rt_array_pop_long(long long *);
extern double rt_array_pop_double(double *);
extern char rt_array_pop_char(char *);
extern int rt_array_pop_bool(int *);
extern unsigned char rt_array_pop_byte(unsigned char *);
extern char *rt_array_pop_string(char **);
extern void *rt_array_pop_ptr(void **);

/* Runtime array concat functions */
extern long long *rt_array_concat_long(RtArena *, long long *, long long *);
extern double *rt_array_concat_double(RtArena *, double *, double *);
extern char *rt_array_concat_char(RtArena *, char *, char *);
extern int *rt_array_concat_bool(RtArena *, int *, int *);
extern unsigned char *rt_array_concat_byte(RtArena *, unsigned char *, unsigned char *);
extern char **rt_array_concat_string(RtArena *, char **, char **);
extern void **rt_array_concat_ptr(RtArena *, void **, void **);

/* Runtime array slice functions (start, end, step) */
extern long long *rt_array_slice_long(RtArena *, long long *, long, long, long);
extern double *rt_array_slice_double(RtArena *, double *, long, long, long);
extern char *rt_array_slice_char(RtArena *, char *, long, long, long);
extern int *rt_array_slice_bool(RtArena *, int *, long, long, long);
extern unsigned char *rt_array_slice_byte(RtArena *, unsigned char *, long, long, long);
extern char **rt_array_slice_string(RtArena *, char **, long, long, long);

/* Runtime array reverse functions */
extern long long *rt_array_rev_long(RtArena *, long long *);
extern double *rt_array_rev_double(RtArena *, double *);
extern char *rt_array_rev_char(RtArena *, char *);
extern int *rt_array_rev_bool(RtArena *, int *);
extern unsigned char *rt_array_rev_byte(RtArena *, unsigned char *);
extern char **rt_array_rev_string(RtArena *, char **);

/* Runtime array remove functions */
extern long long *rt_array_rem_long(RtArena *, long long *, long);
extern double *rt_array_rem_double(RtArena *, double *, long);
extern char *rt_array_rem_char(RtArena *, char *, long);
extern int *rt_array_rem_bool(RtArena *, int *, long);
extern unsigned char *rt_array_rem_byte(RtArena *, unsigned char *, long);
extern char **rt_array_rem_string(RtArena *, char **, long);

/* Runtime array insert functions */
extern long long *rt_array_ins_long(RtArena *, long long *, long long, long);
extern double *rt_array_ins_double(RtArena *, double *, double, long);
extern char *rt_array_ins_char(RtArena *, char *, char, long);
extern int *rt_array_ins_bool(RtArena *, int *, int, long);
extern unsigned char *rt_array_ins_byte(RtArena *, unsigned char *, unsigned char, long);
extern char **rt_array_ins_string(RtArena *, char **, const char *, long);

/* Runtime array push (copy) functions */
extern long long *rt_array_push_copy_long(RtArena *, long long *, long long);
extern double *rt_array_push_copy_double(RtArena *, double *, double);
extern char *rt_array_push_copy_char(RtArena *, char *, char);
extern int *rt_array_push_copy_bool(RtArena *, int *, int);
extern unsigned char *rt_array_push_copy_byte(RtArena *, unsigned char *, unsigned char);
extern char **rt_array_push_copy_string(RtArena *, char **, const char *);

/* Runtime array indexOf functions */
extern long rt_array_indexOf_long(long long *, long long);
extern long rt_array_indexOf_double(double *, double);
extern long rt_array_indexOf_char(char *, char);
extern long rt_array_indexOf_bool(int *, int);
extern long rt_array_indexOf_byte(unsigned char *, unsigned char);
extern long rt_array_indexOf_string(char **, const char *);

/* Runtime array contains functions */
extern int rt_array_contains_long(long long *, long long);
extern int rt_array_contains_double(double *, double);
extern int rt_array_contains_char(char *, char);
extern int rt_array_contains_bool(int *, int);
extern int rt_array_contains_byte(unsigned char *, unsigned char);
extern int rt_array_contains_string(char **, const char *);

/* Runtime array clone functions */
extern long long *rt_array_clone_long(RtArena *, long long *);
extern double *rt_array_clone_double(RtArena *, double *);
extern char *rt_array_clone_char(RtArena *, char *);
extern int *rt_array_clone_bool(RtArena *, int *);
extern unsigned char *rt_array_clone_byte(RtArena *, unsigned char *);
extern char **rt_array_clone_string(RtArena *, char **);

/* Runtime array join functions */
extern char *rt_array_join_long(RtArena *, long long *, const char *);
extern char *rt_array_join_double(RtArena *, double *, const char *);
extern char *rt_array_join_char(RtArena *, char *, const char *);
extern char *rt_array_join_bool(RtArena *, int *, const char *);
extern char *rt_array_join_byte(RtArena *, unsigned char *, const char *);
extern char *rt_array_join_string(RtArena *, char **, const char *);

/* Runtime array create from static data */
extern long long *rt_array_create_long(RtArena *, size_t, const long long *);
extern double *rt_array_create_double(RtArena *, size_t, const double *);
extern char *rt_array_create_char(RtArena *, size_t, const char *);
extern int *rt_array_create_bool(RtArena *, size_t, const int *);
extern unsigned char *rt_array_create_byte(RtArena *, size_t, const unsigned char *);
extern char **rt_array_create_string(RtArena *, size_t, const char **);

/* Runtime array equality functions */
extern int rt_array_eq_long(long long *, long long *);
extern int rt_array_eq_double(double *, double *);
extern int rt_array_eq_char(char *, char *);
extern int rt_array_eq_bool(int *, int *);
extern int rt_array_eq_byte(unsigned char *, unsigned char *);
extern int rt_array_eq_string(char **, char **);

/* Runtime range creation */
extern long long *rt_array_range(RtArena *, long long, long long);

/* Global convenience functions (readLine, println, printErr, printErrLn) */
extern char *rt_read_line(RtArena *);
extern void rt_println(const char *);
extern void rt_print_err(const char *);
extern void rt_print_err_ln(const char *);

/* Byte array extension methods */
extern char *rt_byte_array_to_string(RtArena *, unsigned char *);
extern char *rt_byte_array_to_string_latin1(RtArena *, unsigned char *);
extern char *rt_byte_array_to_hex(RtArena *, unsigned char *);
extern char *rt_byte_array_to_base64(RtArena *, unsigned char *);
extern unsigned char *rt_string_to_bytes(RtArena *, const char *);

/* String splitting methods */
extern char **rt_str_split_whitespace(RtArena *, const char *);
extern char **rt_str_split_lines(RtArena *, const char *);
extern int rt_str_is_blank(const char *);

/* Mutable string operations */
extern char *rt_string_with_capacity(RtArena *, size_t);
extern char *rt_string_from(RtArena *, const char *);
extern char *rt_string_ensure_mutable(RtArena *, char *);
extern char *rt_string_append(char *, const char *);

/* Environment operations */
extern char *rt_env_get(RtArena *, const char *);
extern char **rt_env_names(RtArena *);

/* Forward declarations */
static RtArena *__main_arena__ = NULL;

int main() {
    RtArena *__local_arena__ = rt_arena_create(NULL);
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    RtAny __sn__x = rt_box_bool(1L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_pointer(__local_arena__, __sn__x);
        rt_str_concat(__local_arena__, _p0, "\n");
    });
        rt_print_string(_str_arg0);
    });
    goto main_return;
main_return:
    rt_arena_destroy(__local_arena__);
    return _return_value;
}

