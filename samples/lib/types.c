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
void demo_types(RtArena *);
void show_integers(RtArena *);
void show_doubles(RtArena *);
void show_strings(RtArena *);
void show_chars(RtArena *);
void show_booleans(RtArena *);
void show_type_conversion(RtArena *);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);
static RtAny __thunk_1(void);
static RtAny __thunk_2(void);
static RtAny __thunk_3(void);
static RtAny __thunk_4(void);
static RtAny __thunk_5(void);

void demo_types(RtArena *__caller_arena__) {
    RtArena *__local_arena__ = rt_arena_create(__caller_arena__);
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                      Sindarin Type System                        │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_integers", __args, 0, __thunk_0);
    } else {
        show_integers(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_doubles", __args, 0, __thunk_1);
    } else {
        show_doubles(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_strings", __args, 0, __thunk_2);
    } else {
        show_strings(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_chars", __args, 0, __thunk_3);
    } else {
        show_chars(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_booleans", __args, 0, __thunk_4);
    } else {
        show_booleans(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("show_type_conversion", __args, 0, __thunk_5);
    } else {
        show_type_conversion(__local_arena__);
    }
    (void)0;
});
    goto demo_types_return;
demo_types_return:
    rt_arena_destroy(__local_arena__);
    return;
}

void show_integers(RtArena *__caller_arena__) {
    RtArena *__local_arena__ = rt_arena_create(__caller_arena__);
    rt_print_string("--- 1. Integer Type (int) ---\n");
    long long a = 42LL;
    long long b = -17LL;
    long long c = 0LL;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, a);
        char *_r = rt_str_concat(__local_arena__, "a = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, b);
        char *_r = rt_str_concat(__local_arena__, "b = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, c);
        char *_r = rt_str_concat(__local_arena__, "c = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nArithmetic:\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_add_long(a, b));
        char *_r = rt_str_concat(__local_arena__, "  a + b = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_sub_long(a, b));
        char *_r = rt_str_concat(__local_arena__, "  a - b = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_mul_long(a, 2LL));
        char *_r = rt_str_concat(__local_arena__, "  a * 2 = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_div_long(a, 5LL));
        char *_r = rt_str_concat(__local_arena__, "  a / 5 = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_mod_long(a, 5LL));
        char *_r = rt_str_concat(__local_arena__, "  a % 5 = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nIncrement/Decrement:\n");
    long long x = 5LL;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, x);
        char *_r = rt_str_concat(__local_arena__, "  x = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_post_inc_long(&x);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, x);
        char *_r = rt_str_concat(__local_arena__, "  After x++: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_post_dec_long(&x);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, x);
        char *_r = rt_str_concat(__local_arena__, "  After x--: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nComparisons:\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, 1LL);
        char *_r = rt_str_concat(__local_arena__, "  10 == 10: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, 1LL);
        char *_r = rt_str_concat(__local_arena__, "  10 != 5: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, 1LL);
        char *_r = rt_str_concat(__local_arena__, "  10 > 5: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, 0LL);
        char *_r = rt_str_concat(__local_arena__, "  10 < 5: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, 1LL);
        char *_r = rt_str_concat(__local_arena__, "  10 >= 10: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, 1LL);
        char *_r = rt_str_concat(__local_arena__, "  10 <= 10: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto show_integers_return;
show_integers_return:
    rt_arena_destroy(__local_arena__);
    return;
}

void show_doubles(RtArena *__caller_arena__) {
    RtArena *__local_arena__ = rt_arena_create(__caller_arena__);
    rt_print_string("--- 2. Double Type (double) ---\n");
    double pi = 3.1415899999999999;
    double e = 2.71828;
    double negative = -1.5;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__local_arena__, pi);
        char *_r = rt_str_concat(__local_arena__, "pi = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__local_arena__, e);
        char *_r = rt_str_concat(__local_arena__, "e = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__local_arena__, negative);
        char *_r = rt_str_concat(__local_arena__, "negative = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nArithmetic:\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__local_arena__, rt_add_double(pi, e));
        char *_r = rt_str_concat(__local_arena__, "  pi + e = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__local_arena__, rt_mul_double(pi, 2.0));
        char *_r = rt_str_concat(__local_arena__, "  pi * 2.0 = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__local_arena__, 3.3333333333333335);
        char *_r = rt_str_concat(__local_arena__, "  10.0 / 3.0 = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nMixed operations:\n");
    double radius = 5.0;
    double area = rt_mul_double(rt_mul_double(pi, radius), radius);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__local_arena__, area);
        char *_r = rt_str_concat(__local_arena__, "  Circle area (r=5): ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto show_doubles_return;
show_doubles_return:
    rt_arena_destroy(__local_arena__);
    return;
}

void show_strings(RtArena *__caller_arena__) {
    RtArena *__local_arena__ = rt_arena_create(__caller_arena__);
    rt_print_string("--- 3. String Type (str) ---\n");
    char * greeting = rt_to_string_string(__local_arena__, "Hello");
    char * name = rt_to_string_string(__local_arena__, "World");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "greeting = \"", greeting);
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "name = \"", name);
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    char * message = ({ char *_left = ({ char *_left = rt_str_concat(__local_arena__, greeting, ", "); char *_right = name; char *_res = rt_str_concat(__local_arena__, _left, _right);  _res; }); char *_right = "!"; char *_res = rt_str_concat(__local_arena__, _left, _right);  _res; });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Concatenated: ", message);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    long long age = 25LL;
    double height = 5.9000000000000004;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, age);
        char *_p1 = rt_to_string_double(__local_arena__, height);
        char *_r = rt_str_concat(__local_arena__, "Interpolation: Age is ", _p0);
        _r = rt_str_concat(__local_arena__, _r, ", height is ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    char * empty = rt_to_string_string(__local_arena__, "");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "Empty string: \"", empty);
        _r = rt_str_concat(__local_arena__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nString comparisons:\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_eq_string("abc", "abc"));
        char *_r = rt_str_concat(__local_arena__, "  \"abc\" == \"abc\": ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_ne_string("abc", "xyz"));
        char *_r = rt_str_concat(__local_arena__, "  \"abc\" != \"xyz\": ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, rt_lt_string("abc", "abd"));
        char *_r = rt_str_concat(__local_arena__, "  \"abc\" < \"abd\": ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto show_strings_return;
show_strings_return:
    rt_arena_destroy(__local_arena__);
    return;
}

void show_chars(RtArena *__caller_arena__) {
    RtArena *__local_arena__ = rt_arena_create(__caller_arena__);
    rt_print_string("--- 4. Character Type (char) ---\n");
    char letter = 'A';
    char digit = '7';
    char symbol = '@';
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_char(__local_arena__, letter);
        char *_r = rt_str_concat(__local_arena__, "letter = '", _p0);
        _r = rt_str_concat(__local_arena__, _r, "'\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_char(__local_arena__, digit);
        char *_r = rt_str_concat(__local_arena__, "digit = '", _p0);
        _r = rt_str_concat(__local_arena__, _r, "'\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_char(__local_arena__, symbol);
        char *_r = rt_str_concat(__local_arena__, "symbol = '", _p0);
        _r = rt_str_concat(__local_arena__, _r, "'\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    char tab = '\t';
    rt_print_string("\nEscape sequences:\n");
    rt_print_string("  Tab:");
    rt_print_char(tab);
    rt_print_string("between\n");
    char first = 'S';
    char * rest = rt_to_string_string(__local_arena__, "indarin");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_char(__local_arena__, first);
        char *_r = rt_str_concat(__local_arena__, "  Combined: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, rest);
        _r = rt_str_concat(__local_arena__, _r, "\n\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto show_chars_return;
show_chars_return:
    rt_arena_destroy(__local_arena__);
    return;
}

void show_booleans(RtArena *__caller_arena__) {
    RtArena *__local_arena__ = rt_arena_create(__caller_arena__);
    rt_print_string("--- 5. Boolean Type (bool) ---\n");
    bool is_active = 1L;
    bool is_complete = 0L;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, is_active);
        char *_r = rt_str_concat(__local_arena__, "is_active = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, is_complete);
        char *_r = rt_str_concat(__local_arena__, "is_complete = ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    long long x = 10LL;
    long long y = 5LL;
    bool greater = rt_gt_long(x, y);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, x);
        char *_p1 = rt_to_string_long(__local_arena__, y);
        char *_p2 = rt_to_string_bool(__local_arena__, greater);
        char *_r = rt_str_concat(__local_arena__, "\n", _p0);
        _r = rt_str_concat(__local_arena__, _r, " > ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, " = ");
        _r = rt_str_concat(__local_arena__, _r, _p2);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nNOT operator (!):\n");
    bool flag = 0L;
    if (rt_not_bool(flag)) {
        {
            rt_print_string("  !false = true\n");
        }
    }
    (flag = 1L);
    if (rt_not_bool(flag)) {
        {
            rt_print_string("  never printed\n");
        }
    }
    else {
        {
            rt_print_string("  !true = false\n\n");
        }
    }
    goto show_booleans_return;
show_booleans_return:
    rt_arena_destroy(__local_arena__);
    return;
}

void show_type_conversion(RtArena *__caller_arena__) {
    RtArena *__local_arena__ = rt_arena_create(__caller_arena__);
    rt_print_string("--- 6. Type Display in Strings ---\n");
    long long i = 42LL;
    double d = 3.1400000000000001;
    char * s = rt_to_string_string(__local_arena__, "hello");
    char c = 'X';
    bool b = 1L;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, i);
        char *_r = rt_str_concat(__local_arena__, "int: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__local_arena__, d);
        char *_r = rt_str_concat(__local_arena__, "double: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, "str: ", s);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_char(__local_arena__, c);
        char *_r = rt_str_concat(__local_arena__, "char: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__local_arena__, b);
        char *_r = rt_str_concat(__local_arena__, "bool: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, i);
        char *_p1 = rt_to_string_double(__local_arena__, d);
        char *_p2 = rt_to_string_char(__local_arena__, c);
        char *_p3 = rt_to_string_bool(__local_arena__, b);
        char *_r = rt_str_concat(__local_arena__, "\nMixed: i=", _p0);
        _r = rt_str_concat(__local_arena__, _r, ", d=");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, ", s=");
        _r = rt_str_concat(__local_arena__, _r, s);
        _r = rt_str_concat(__local_arena__, _r, ", c=");
        _r = rt_str_concat(__local_arena__, _r, _p2);
        _r = rt_str_concat(__local_arena__, _r, ", b=");
        _r = rt_str_concat(__local_arena__, _r, _p3);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto show_type_conversion_return;
show_type_conversion_return:
    rt_arena_destroy(__local_arena__);
    return;
}

int main() {
    RtArena *__local_arena__ = rt_arena_create(NULL);
    int _return_value = 0;
    goto main_return;
main_return:
    rt_arena_destroy(__local_arena__);
    return _return_value;
}

/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    show_integers((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_1(void) {
    show_doubles((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_2(void) {
    show_strings((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_3(void) {
    show_chars((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_4(void) {
    show_booleans((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_5(void) {
    show_type_conversion((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

