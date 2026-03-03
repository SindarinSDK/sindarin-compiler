#ifndef RUNTIME_STRING_H
#define RUNTIME_STRING_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include "runtime/arena/arena_v2.h"
#include "runtime/array/runtime_array_v2.h"

/* ============================================================================
 * Print Functions
 * ============================================================================ */

void rt_print_long(long long val);
void rt_print_double(double val);
void rt_print_char(long c);
void rt_print_bool(long b);
void rt_print_byte(unsigned char b);

#endif /* RUNTIME_STRING_H */
