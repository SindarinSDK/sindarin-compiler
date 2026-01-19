// Helper C file for variadic native function test
// This provides a custom variadic function to avoid conflicts with stdio.h

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

// Custom printf wrapper that matches Sindarin's type mapping
// - format: char * (Sindarin str maps to char *)
// - return: int32_t (Sindarin int32 maps to int32_t)
int32_t test_printf(char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    return (int32_t)result;
}
