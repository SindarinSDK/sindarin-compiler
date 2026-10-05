#include <stdarg.h>

long long sn_variadic_sum(int32_t count, ...) {
    va_list args;
    va_start(args, count);
    long long result = 0;
    for (int32_t i = 0; i < count; i++) result += va_arg(args, long long);
    va_end(args);
    return result;
}
