#ifndef SN_CONV_H
#define SN_CONV_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

/* ---- char methods ---- */

static inline char *sn_char_to_string(char c)
{
    char *s = malloc(2);
    s[0] = c; s[1] = '\0';
    return s;
}

#define __sn__char_toString(ptr) sn_char_to_string(*(ptr))
#define __sn__char_toUpper(ptr) ((char)toupper((unsigned char)(*(ptr))))
#define __sn__char_toLower(ptr) ((char)tolower((unsigned char)(*(ptr))))
#define __sn__char_toInt(ptr) ((long long)(*(ptr)))
#define __sn__char_isDigit(ptr) ((bool)isdigit((unsigned char)(*(ptr))))
#define __sn__char_isAlpha(ptr) ((bool)isalpha((unsigned char)(*(ptr))))
#define __sn__char_isWhitespace(ptr) ((bool)isspace((unsigned char)(*(ptr))))
#define __sn__char_isAlnum(ptr) ((bool)isalnum((unsigned char)(*(ptr))))

/* ---- int methods ---- */

#define __sn__int_toDouble(ptr) ((double)(*(ptr)))
#define __sn__int_toLong(ptr) ((long long)(*(ptr)))
#define __sn__int_toUint(ptr) ((unsigned long long)(*(ptr)))
#define __sn__int_toByte(ptr) ((unsigned char)(*(ptr)))
#define __sn__int_toChar(ptr) ((char)(*(ptr)))

/* ---- long methods ---- */

#define __sn__long_toInt(ptr) ((long long)(*(ptr)))
#define __sn__long_toDouble(ptr) ((double)(*(ptr)))

/* ---- double methods ---- */

#define __sn__double_toInt(ptr) ((long long)(*(ptr)))
#define __sn__double_toLong(ptr) ((long long)(*(ptr)))

/* ---- uint methods ---- */

#define __sn__uint_toInt(ptr) ((long long)(*(ptr)))
#define __sn__uint_toLong(ptr) ((long long)(*(ptr)))
#define __sn__uint_toDouble(ptr) ((double)(*(ptr)))

/* ---- byte methods ---- */

#define __sn__byte_toInt(ptr) ((long long)(*(ptr)))
#define __sn__byte_toChar(ptr) ((char)(*(ptr)))

/* ---- bool methods ---- */

#define __sn__bool_toInt(ptr) ((long long)(*(ptr)))

/* ---- str methods (type-prefixed conversions) ---- */

#define __sn__str_toInt(ptr) ((long long)strtoll(*(ptr), NULL, 10))
#define __sn__str_toDouble(ptr) strtod(*(ptr), NULL)
#define __sn__str_toLong(ptr) ((long long)strtoll(*(ptr), NULL, 10))

/* ---- Comparison helpers ---- */

static inline bool sn_eq_long(long long a, long long b) { return a == b; }
static inline bool sn_lt_long(long long a, long long b) { return a < b; }
static inline bool sn_gt_long(long long a, long long b) { return a > b; }
static inline bool sn_le_long(long long a, long long b) { return a <= b; }
static inline bool sn_ge_long(long long a, long long b) { return a >= b; }

static inline bool sn_eq_double(double a, double b) { return a == b; }
static inline bool sn_lt_double(double a, double b) { return a < b; }
static inline bool sn_gt_double(double a, double b) { return a > b; }
static inline bool sn_le_double(double a, double b) { return a <= b; }
static inline bool sn_ge_double(double a, double b) { return a >= b; }

static inline bool sn_eq_float(float a, float b) { return a == b; }
static inline bool sn_lt_float(float a, float b) { return a < b; }
static inline bool sn_gt_float(float a, float b) { return a > b; }
static inline bool sn_le_float(float a, float b) { return a <= b; }
static inline bool sn_ge_float(float a, float b) { return a >= b; }

static inline bool sn_lt_char(char a, char b) { return a < b; }
static inline bool sn_gt_char(char a, char b) { return a > b; }
static inline bool sn_le_char(char a, char b) { return a <= b; }
static inline bool sn_ge_char(char a, char b) { return a >= b; }

static inline bool sn_lt_byte(unsigned char a, unsigned char b) { return a < b; }
static inline bool sn_gt_byte(unsigned char a, unsigned char b) { return a > b; }
static inline bool sn_le_byte(unsigned char a, unsigned char b) { return a <= b; }
static inline bool sn_ge_byte(unsigned char a, unsigned char b) { return a >= b; }

static inline bool sn_lt_int32(int a, int b) { return a < b; }
static inline bool sn_gt_int32(int a, int b) { return a > b; }
static inline bool sn_le_int32(int a, int b) { return a <= b; }
static inline bool sn_ge_int32(int a, int b) { return a >= b; }

static inline bool sn_lt_uint(unsigned long long a, unsigned long long b) { return a < b; }
static inline bool sn_gt_uint(unsigned long long a, unsigned long long b) { return a > b; }
static inline bool sn_le_uint(unsigned long long a, unsigned long long b) { return a <= b; }
static inline bool sn_ge_uint(unsigned long long a, unsigned long long b) { return a >= b; }

static inline bool sn_lt_uint32(unsigned int a, unsigned int b) { return a < b; }
static inline bool sn_gt_uint32(unsigned int a, unsigned int b) { return a > b; }
static inline bool sn_le_uint32(unsigned int a, unsigned int b) { return a <= b; }
static inline bool sn_ge_uint32(unsigned int a, unsigned int b) { return a >= b; }

/* ---- I/O ---- */

static inline void sn_print(const char *s)
{
    if (s) fputs(s, stdout);
}

static inline void sn_println(const char *s)
{
    if (s) puts(s);
    else puts("");
}

/* ---- Assertions ---- */

static inline void sn_assert(bool condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message ? message : "Assertion failed");
        exit(1);
    }
}

/* ---- Exit ---- */

static inline void sn_exit(int code)
{
    exit(code);
}

#endif
