#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"
void __sn__test_string_format();
void __sn__test_int_format();
void __sn__test_double_format();
void __sn__test_char_format();
void __sn__test_bool_format();
void __sn__test_mixed_format();
void __sn__test_no_args_format();
void __sn__test_width_precision();


void __sn__test_string_format() {
    sn_print("  Testing printf with string arguments...\n");
    test_printf("    Hello, %s!\n", "World");
    test_printf("    Name: %s, Language: %s\n", "Alice", "Sindarin");
}


void __sn__test_int_format() {
    sn_print("  Testing printf with integer arguments...\n");
    long long __sn__x = 42LL;
    long long __sn__y = (-17LL);
    long long __sn__big = 1000000LL;
    test_printf("    Integer: %ld\n", __sn__x);
    test_printf("    Negative: %ld\n", __sn__y);
    test_printf("    Large: %ld\n", __sn__big);
    test_printf("    Multiple ints: %ld, %ld, %ld\n", 1LL, 2LL, 3LL);
}


void __sn__test_double_format() {
    sn_print("  Testing printf with double arguments...\n");
    double __sn__pi = 3.1415926535900001;
    double __sn__e = 2.71828;
    test_printf("    Pi: %f\n", __sn__pi);
    test_printf("    e: %.5f\n", __sn__e);
    test_printf("    Formatted: %.2f\n", 123.456);
}


void __sn__test_char_format() {
    sn_print("  Testing printf with char arguments...\n");
    char __sn__c = (char)88;
    test_printf("    Char: %c\n", __sn__c);
    test_printf("    Multiple chars: %c%c%c\n", (char)65, (char)66, (char)67);
}


void __sn__test_bool_format() {
    sn_print("  Testing printf with bool arguments (as int)...\n");
    bool __sn__t = true;
    bool __sn__f = false;
    test_printf("    True: %ld\n", __sn__t);
    test_printf("    False: %ld\n", __sn__f);
}


void __sn__test_mixed_format() {
    sn_print("  Testing printf with mixed argument types...\n");
    sn_auto_str char * __sn__name = strdup("Test");
    long long __sn__count = 42LL;
    double __sn__value = 3.1400000000000001;
    char __sn__flag = (char)42;
    test_printf("    String: %s, Int: %ld, Double: %.2f, Char: %c\n", __sn__name, __sn__count, __sn__value, __sn__flag);
    test_printf("    Combined: %s scored %ld points with %.1f%% accuracy\n", "Player", 100LL, 95.5);
}


void __sn__test_no_args_format() {
    sn_print("  Testing printf with no variadic arguments...\n");
    test_printf("    Just a plain string\n");
}


void __sn__test_width_precision() {
    sn_print("  Testing printf with width and precision...\n");
    test_printf("    Right aligned: %10s|\n", "test");
    test_printf("    Left aligned: %-10s|\n", "test");
    test_printf("    Zero padded: %05ld\n", 42LL);
    test_printf("    Precision: %.3f\n", 1.2345678899999999);
}

int main() {
    sn_print("=== Variadic Function Interop Test ===\n\n");
    __sn__test_string_format();
    __sn__test_int_format();
    __sn__test_double_format();
    __sn__test_char_format();
    __sn__test_bool_format();
    __sn__test_mixed_format();
    __sn__test_no_args_format();
    __sn__test_width_precision();
    sn_print("\n=== All variadic interop tests PASSED! ===\n");
    return 0LL;}
