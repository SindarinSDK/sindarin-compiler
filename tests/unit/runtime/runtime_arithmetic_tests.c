// tests/unit/runtime/runtime_arithmetic_tests.c
// Tests for runtime arithmetic operations with overflow checking

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <float.h>
#include "../runtime.h"
#include "../test_harness.h"

/* ============================================================================
 * Long Arithmetic Tests
 * ============================================================================ */

static void test_rt_add_long_basic(void)
{
    assert(rt_add_long(1, 2) == 3);
    assert(rt_add_long(0, 0) == 0);
    assert(rt_add_long(-1, 1) == 0);
    assert(rt_add_long(-5, -3) == -8);
    assert(rt_add_long(100, -50) == 50);
    assert(rt_add_long(LLONG_MAX - 1, 1) == LLONG_MAX);
    assert(rt_add_long(LLONG_MIN + 1, -1) == LLONG_MIN);
}

static void test_rt_sub_long_basic(void)
{
    assert(rt_sub_long(5, 3) == 2);
    assert(rt_sub_long(0, 0) == 0);
    assert(rt_sub_long(-1, -1) == 0);
    assert(rt_sub_long(10, -5) == 15);
    assert(rt_sub_long(-10, 5) == -15);
    assert(rt_sub_long(LLONG_MIN + 1, 1) == LLONG_MIN);
    assert(rt_sub_long(LLONG_MAX - 1, -1) == LLONG_MAX);
}

static void test_rt_mul_long_basic(void)
{
    assert(rt_mul_long(3, 4) == 12);
    assert(rt_mul_long(0, 100) == 0);
    assert(rt_mul_long(100, 0) == 0);
    assert(rt_mul_long(-2, 3) == -6);
    assert(rt_mul_long(2, -3) == -6);
    assert(rt_mul_long(-2, -3) == 6);
    assert(rt_mul_long(1, 1000000) == 1000000);
    assert(rt_mul_long(-1, 1000000) == -1000000);
    assert(rt_mul_long(-1000000, -1) == 1000000);
}

static void test_rt_div_long_basic(void)
{
    assert(rt_div_long(10, 2) == 5);
    assert(rt_div_long(10, 3) == 3);
    assert(rt_div_long(-10, 2) == -5);
    assert(rt_div_long(10, -2) == -5);
    assert(rt_div_long(-10, -2) == 5);
    assert(rt_div_long(0, 5) == 0);
    assert(rt_div_long(1000000, 1) == 1000000);
    assert(rt_div_long(-1000000, 1) == -1000000);
}

static void test_rt_mod_long_basic(void)
{
    assert(rt_mod_long(10, 3) == 1);
    assert(rt_mod_long(10, 5) == 0);
    assert(rt_mod_long(0, 5) == 0);
    assert(rt_mod_long(-10, 3) == -1);
    assert(rt_mod_long(10, -3) == 1);
    assert(rt_mod_long(-10, -3) == -1);
}

static void test_rt_neg_long_basic(void)
{
    assert(rt_neg_long(5) == -5);
    assert(rt_neg_long(-5) == 5);
    assert(rt_neg_long(0) == 0);
    assert(rt_neg_long(LLONG_MAX) == -LLONG_MAX);
}

/* ============================================================================
 * Long Comparison Tests (inline functions)
 * ============================================================================ */

static void test_rt_long_comparisons(void)
{
    /* Equal */
    assert(rt_eq_long(5, 5) == 1);
    assert(rt_eq_long(5, 6) == 0);
    assert(rt_eq_long(-5, -5) == 1);

    /* Not equal */
    assert(rt_ne_long(5, 6) == 1);
    assert(rt_ne_long(5, 5) == 0);

    /* Less than */
    assert(rt_lt_long(3, 5) == 1);
    assert(rt_lt_long(5, 3) == 0);
    assert(rt_lt_long(5, 5) == 0);
    assert(rt_lt_long(-5, -3) == 1);

    /* Less than or equal */
    assert(rt_le_long(3, 5) == 1);
    assert(rt_le_long(5, 5) == 1);
    assert(rt_le_long(5, 3) == 0);

    /* Greater than */
    assert(rt_gt_long(5, 3) == 1);
    assert(rt_gt_long(3, 5) == 0);
    assert(rt_gt_long(5, 5) == 0);

    /* Greater than or equal */
    assert(rt_ge_long(5, 3) == 1);
    assert(rt_ge_long(5, 5) == 1);
    assert(rt_ge_long(3, 5) == 0);
}

/* ============================================================================
 * Double Arithmetic Tests
 * ============================================================================ */

static void test_rt_add_double_basic(void)
{
    assert(rt_add_double(1.5, 2.5) == 4.0);
    assert(rt_add_double(0.0, 0.0) == 0.0);
    assert(rt_add_double(-1.5, 1.5) == 0.0);
    assert(rt_add_double(-5.0, -3.0) == -8.0);

    /* Test with very small numbers */
    double small = rt_add_double(0.1, 0.2);
    assert(fabs(small - 0.3) < 0.0001);
}

static void test_rt_sub_double_basic(void)
{
    assert(rt_sub_double(5.0, 3.0) == 2.0);
    assert(rt_sub_double(0.0, 0.0) == 0.0);
    assert(rt_sub_double(-1.5, -1.5) == 0.0);
    assert(rt_sub_double(10.5, -5.5) == 16.0);
}

static void test_rt_mul_double_basic(void)
{
    assert(rt_mul_double(3.0, 4.0) == 12.0);
    assert(rt_mul_double(0.0, 100.0) == 0.0);
    assert(rt_mul_double(-2.0, 3.0) == -6.0);
    assert(rt_mul_double(-2.0, -3.0) == 6.0);
    assert(rt_mul_double(0.5, 2.0) == 1.0);
}

static void test_rt_div_double_basic(void)
{
    assert(rt_div_double(10.0, 2.0) == 5.0);
    assert(rt_div_double(10.0, 4.0) == 2.5);
    assert(rt_div_double(-10.0, 2.0) == -5.0);
    assert(rt_div_double(1.0, 3.0) - 0.333333 < 0.0001);
}

static void test_rt_neg_double_basic(void)
{
    assert(rt_neg_double(5.0) == -5.0);
    assert(rt_neg_double(-5.0) == 5.0);
    assert(rt_neg_double(0.0) == 0.0);
    assert(rt_neg_double(DBL_MAX) == -DBL_MAX);
}

/* ============================================================================
 * Double Comparison Tests (inline functions)
 * ============================================================================ */

static void test_rt_double_comparisons(void)
{
    /* Equal */
    assert(rt_eq_double(5.0, 5.0) == 1);
    assert(rt_eq_double(5.0, 5.1) == 0);

    /* Not equal */
    assert(rt_ne_double(5.0, 5.1) == 1);
    assert(rt_ne_double(5.0, 5.0) == 0);

    /* Less than */
    assert(rt_lt_double(3.0, 5.0) == 1);
    assert(rt_lt_double(5.0, 3.0) == 0);
    assert(rt_lt_double(5.0, 5.0) == 0);

    /* Less than or equal */
    assert(rt_le_double(3.0, 5.0) == 1);
    assert(rt_le_double(5.0, 5.0) == 1);
    assert(rt_le_double(5.0, 3.0) == 0);

    /* Greater than */
    assert(rt_gt_double(5.0, 3.0) == 1);
    assert(rt_gt_double(3.0, 5.0) == 0);
    assert(rt_gt_double(5.0, 5.0) == 0);

    /* Greater than or equal */
    assert(rt_ge_double(5.0, 3.0) == 1);
    assert(rt_ge_double(5.0, 5.0) == 1);
    assert(rt_ge_double(3.0, 5.0) == 0);
}

/* ============================================================================
 * Boolean Operation Tests
 * ============================================================================ */

static void test_rt_not_bool(void)
{
    assert(rt_not_bool(0) == 1);
    assert(rt_not_bool(1) == 0);
    assert(rt_not_bool(42) == 0);  /* Any non-zero is truthy */
    assert(rt_not_bool(-1) == 0);
}

/* ============================================================================
 * Post Increment/Decrement Tests
 * ============================================================================ */

static void test_rt_post_inc_long(void)
{
    long long val = 5;
    long long result = rt_post_inc_long(&val);
    assert(result == 5);   /* Returns old value */
    assert(val == 6);      /* Variable is incremented */

    val = 0;
    result = rt_post_inc_long(&val);
    assert(result == 0);
    assert(val == 1);

    val = -1;
    result = rt_post_inc_long(&val);
    assert(result == -1);
    assert(val == 0);

    /* Test near max (but not at max to avoid overflow exit) */
    val = LLONG_MAX - 1;
    result = rt_post_inc_long(&val);
    assert(result == LLONG_MAX - 1);
    assert(val == LLONG_MAX);
}

static void test_rt_post_dec_long(void)
{
    long long val = 5;
    long long result = rt_post_dec_long(&val);
    assert(result == 5);   /* Returns old value */
    assert(val == 4);      /* Variable is decremented */

    val = 1;
    result = rt_post_dec_long(&val);
    assert(result == 1);
    assert(val == 0);

    val = 0;
    result = rt_post_dec_long(&val);
    assert(result == 0);
    assert(val == -1);

    /* Test near min (but not at min to avoid overflow exit) */
    val = LLONG_MIN + 1;
    result = rt_post_dec_long(&val);
    assert(result == LLONG_MIN + 1);
    assert(val == LLONG_MIN);
}

/* ============================================================================
 * String Comparison Tests (inline functions)
 * ============================================================================ */

static void test_rt_string_comparisons(void)
{
    /* Equal */
    assert(rt_eq_string("hello", "hello") == 1);
    assert(rt_eq_string("hello", "world") == 0);
    assert(rt_eq_string("", "") == 1);

    /* Not equal */
    assert(rt_ne_string("hello", "world") == 1);
    assert(rt_ne_string("hello", "hello") == 0);

    /* Less than (lexicographic) */
    assert(rt_lt_string("apple", "banana") == 1);
    assert(rt_lt_string("banana", "apple") == 0);
    assert(rt_lt_string("abc", "abd") == 1);
    assert(rt_lt_string("abc", "abc") == 0);

    /* Less than or equal */
    assert(rt_le_string("apple", "banana") == 1);
    assert(rt_le_string("apple", "apple") == 1);
    assert(rt_le_string("banana", "apple") == 0);

    /* Greater than */
    assert(rt_gt_string("banana", "apple") == 1);
    assert(rt_gt_string("apple", "banana") == 0);
    assert(rt_gt_string("apple", "apple") == 0);

    /* Greater than or equal */
    assert(rt_ge_string("banana", "apple") == 1);
    assert(rt_ge_string("apple", "apple") == 1);
    assert(rt_ge_string("apple", "banana") == 0);
}

/* ============================================================================
 * String Blank Check Tests
 * ============================================================================ */

static void test_rt_str_is_blank(void)
{
    /* Blank strings */
    assert(rt_str_is_blank(NULL) == 1);
    assert(rt_str_is_blank("") == 1);
    assert(rt_str_is_blank(" ") == 1);
    assert(rt_str_is_blank("  ") == 1);
    assert(rt_str_is_blank("\t") == 1);
    assert(rt_str_is_blank("\n") == 1);
    assert(rt_str_is_blank("\r") == 1);
    assert(rt_str_is_blank(" \t\n\r\v\f") == 1);

    /* Non-blank strings */
    assert(rt_str_is_blank("a") == 0);
    assert(rt_str_is_blank(" a") == 0);
    assert(rt_str_is_blank("a ") == 0);
    assert(rt_str_is_blank(" a ") == 0);
    assert(rt_str_is_blank("hello world") == 0);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_arithmetic_main(void)
{
    TEST_SECTION("Runtime Arithmetic");

    /* Long arithmetic */
    TEST_RUN("rt_add_long_basic", test_rt_add_long_basic);
    TEST_RUN("rt_sub_long_basic", test_rt_sub_long_basic);
    TEST_RUN("rt_mul_long_basic", test_rt_mul_long_basic);
    TEST_RUN("rt_div_long_basic", test_rt_div_long_basic);
    TEST_RUN("rt_mod_long_basic", test_rt_mod_long_basic);
    TEST_RUN("rt_neg_long_basic", test_rt_neg_long_basic);
    TEST_RUN("rt_long_comparisons", test_rt_long_comparisons);

    /* Double arithmetic */
    TEST_RUN("rt_add_double_basic", test_rt_add_double_basic);
    TEST_RUN("rt_sub_double_basic", test_rt_sub_double_basic);
    TEST_RUN("rt_mul_double_basic", test_rt_mul_double_basic);
    TEST_RUN("rt_div_double_basic", test_rt_div_double_basic);
    TEST_RUN("rt_neg_double_basic", test_rt_neg_double_basic);
    TEST_RUN("rt_double_comparisons", test_rt_double_comparisons);

    /* Boolean */
    TEST_RUN("rt_not_bool", test_rt_not_bool);

    /* Increment/decrement */
    TEST_RUN("rt_post_inc_long", test_rt_post_inc_long);
    TEST_RUN("rt_post_dec_long", test_rt_post_dec_long);

    /* String comparisons */
    TEST_RUN("rt_string_comparisons", test_rt_string_comparisons);

    /* String utilities */
    TEST_RUN("rt_str_is_blank", test_rt_str_is_blank);
}
