#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

int main() {
    test_printf("Hello, variadic!\n");
    test_printf("Name: %s\n", "World");
    test_printf("Count: %ld\n", 42LL);
    test_printf("Pi: %f\n", 3.1415899999999999);
    test_printf("Int: %ld, Double: %f, String: %s\n", 100LL, 2.718, "test");
    test_printf("Char: %c\n", (char)88);
    test_printf("Bool: %ld\n", true);
    return 0LL;}
