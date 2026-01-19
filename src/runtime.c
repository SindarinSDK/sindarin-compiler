#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include <time.h>

#ifdef _WIN32
    #if defined(__MINGW32__) || defined(__MINGW64__)
    /* MinGW is POSIX-compatible */
    #include <sys/stat.h>
    #include <unistd.h>
    #include <sys/time.h>
    #else
    #include "platform/compat_windows.h"
    #endif
#else
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#endif

#include "runtime.h"
#include "runtime/runtime_string.h"
#include "runtime/runtime_thread.h"

long long rt_add_long(long long a, long long b)
{
    if ((b > 0 && a > LLONG_MAX - b) || (b < 0 && a < LLONG_MIN - b))
    {
        rt_thread_panic("rt_add_long: overflow detected");
    }
    return a + b;
}

long long rt_sub_long(long long a, long long b)
{
    if ((b > 0 && a < LLONG_MIN + b) || (b < 0 && a > LLONG_MAX + b))
    {
        rt_thread_panic("rt_sub_long: overflow detected");
    }
    return a - b;
}

long long rt_mul_long(long long a, long long b)
{
    if (a == 0 || b == 0)
    {
        return 0;
    }

    if ((a > 0 && b > 0) || (a < 0 && b < 0))
    {
        /* Result will be positive, check against LLONG_MAX */
        if (a > 0)
        {
            if (a > LLONG_MAX / b)
            {
                rt_thread_panic("rt_mul_long: overflow detected");
            }
        }
        else
        {
            /* Both negative: need (-a) * (-b) <= LLONG_MAX */
            /* Special case: LLONG_MIN cannot be negated */
            if (a == LLONG_MIN || b == LLONG_MIN)
            {
                rt_thread_panic("rt_mul_long: overflow detected");
            }
            if ((-a) > LLONG_MAX / (-b))
            {
                rt_thread_panic("rt_mul_long: overflow detected");
            }
        }
    }
    else
    {
        /* Result will be negative, check against LLONG_MIN */
        if (a > 0)
        {
            /* a > 0, b < 0 */
            if (b < LLONG_MIN / a)
            {
                rt_thread_panic("rt_mul_long: overflow detected");
            }
        }
        else
        {
            /* a < 0, b > 0 */
            if (a < LLONG_MIN / b)
            {
                rt_thread_panic("rt_mul_long: overflow detected");
            }
        }
    }

    return a * b;
}

long long rt_div_long(long long a, long long b)
{
    if (b == 0)
    {
        rt_thread_panic("Division by zero");
    }
    if (a == LLONG_MIN && b == -1)
    {
        rt_thread_panic("rt_div_long: overflow detected (LLONG_MIN / -1)");
    }
    return a / b;
}

long long rt_mod_long(long long a, long long b)
{
    if (b == 0)
    {
        rt_thread_panic("Modulo by zero");
    }
    if (a == LLONG_MIN && b == -1)
    {
        rt_thread_panic("rt_mod_long: overflow detected (LLONG_MIN % -1)");
    }
    return a % b;
}

/* rt_eq_long, rt_ne_long, rt_lt_long, rt_le_long, rt_gt_long, rt_ge_long
 * are defined as static inline in runtime.h for inlining optimization */

double rt_add_double(double a, double b)
{
    double result = a + b;
    if (isinf(result) && !isinf(a) && !isinf(b))
    {
        rt_thread_panic("rt_add_double: overflow to infinity");
    }
    return result;
}

double rt_sub_double(double a, double b)
{
    double result = a - b;
    if (isinf(result) && !isinf(a) && !isinf(b))
    {
        rt_thread_panic("rt_sub_double: overflow to infinity");
    }
    return result;
}

double rt_mul_double(double a, double b)
{
    double result = a * b;
    if (isinf(result) && !isinf(a) && !isinf(b))
    {
        rt_thread_panic("rt_mul_double: overflow to infinity");
    }
    return result;
}

double rt_div_double(double a, double b)
{
    if (b == 0.0)
    {
        rt_thread_panic("Division by zero");
    }
    double result = a / b;
    if (isinf(result) && !isinf(a) && b != 0.0)
    {
        rt_thread_panic("rt_div_double: overflow to infinity");
    }
    return result;
}

/* rt_eq_double, rt_ne_double, rt_lt_double, rt_le_double, rt_gt_double, rt_ge_double
 * are defined as static inline in runtime.h for inlining optimization */

long long rt_neg_long(long long a)
{
    if (a == LLONG_MIN)
    {
        rt_thread_panic("rt_neg_long: overflow detected (-LLONG_MIN)");
    }
    return -a;
}

double rt_neg_double(double a) { return -a; }

/* rt_not_bool is defined as static inline in runtime.h */

long long rt_post_inc_long(long long *p)
{
    if (p == NULL)
    {
        fprintf(stderr, "rt_post_inc_long: NULL pointer\n");
        exit(1);
    }
    if (*p == LLONG_MAX)
    {
        fprintf(stderr, "rt_post_inc_long: overflow detected\n");
        exit(1);
    }
    return (*p)++;
}

long long rt_post_dec_long(long long *p)
{
    if (p == NULL)
    {
        fprintf(stderr, "rt_post_dec_long: NULL pointer\n");
        exit(1);
    }
    if (*p == LLONG_MIN)
    {
        fprintf(stderr, "rt_post_dec_long: overflow detected\n");
        exit(1);
    }
    return (*p)--;
}

/* ============================================================================
 * String Splitting Methods
 * ============================================================================ */

/* Helper: Check if character is whitespace */
static int is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

/* Split string on any whitespace character */
char **rt_str_split_whitespace(RtArena *arena, const char *str) {
    if (str == NULL) {
        return rt_create_string_array(arena, 4);
    }

    char **result = rt_create_string_array(arena, 16);
    const char *p = str;

    while (*p) {
        /* Skip leading whitespace */
        while (*p && is_whitespace(*p)) {
            p++;
        }

        if (*p == '\0') break;

        /* Find end of word */
        const char *start = p;
        while (*p && !is_whitespace(*p)) {
            p++;
        }

        /* Add word to result */
        size_t len = p - start;
        char *word = rt_arena_alloc(arena, len + 1);
        memcpy(word, start, len);
        word[len] = '\0';
        result = rt_push_string_to_array(arena, result, word);
    }

    return result;
}

/* Split string on line endings (\n, \r\n, \r) */
char **rt_str_split_lines(RtArena *arena, const char *str) {
    if (str == NULL) {
        return rt_create_string_array(arena, 4);
    }

    char **result = rt_create_string_array(arena, 16);
    const char *p = str;
    const char *start = str;

    while (*p) {
        if (*p == '\n') {
            /* Unix line ending or end of Windows \r\n */
            size_t len = p - start;
            char *line = rt_arena_alloc(arena, len + 1);
            memcpy(line, start, len);
            line[len] = '\0';
            result = rt_push_string_to_array(arena, result, line);
            p++;
            start = p;
        } else if (*p == '\r') {
            /* Carriage return - check for \r\n or standalone \r */
            size_t len = p - start;
            char *line = rt_arena_alloc(arena, len + 1);
            memcpy(line, start, len);
            line[len] = '\0';
            result = rt_push_string_to_array(arena, result, line);
            p++;
            if (*p == '\n') {
                /* Windows \r\n - skip the \n too */
                p++;
            }
            start = p;
        } else {
            p++;
        }
    }

    /* Add final line if there's remaining content */
    if (p > start) {
        size_t len = p - start;
        char *line = rt_arena_alloc(arena, len + 1);
        memcpy(line, start, len);
        line[len] = '\0';
        result = rt_push_string_to_array(arena, result, line);
    }

    return result;
}

/* Check if string is empty or contains only whitespace */
int rt_str_is_blank(const char *str) {
    if (str == NULL || *str == '\0') {
        return 1;  /* NULL or empty string is blank */
    }

    const char *p = str;
    while (*p) {
        if (!is_whitespace(*p)) {
            return 0;  /* Found non-whitespace character */
        }
        p++;
    }

    return 1;  /* All characters were whitespace */
}
