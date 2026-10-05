#include <stdarg.h>

bool check_promotions(int32_t tag, ...) {
    va_list args;
    va_start(args, tag);
    bool ok = tag == 1;
    ok &= va_arg(args, long long) == -7000000000LL;
    ok &= va_arg(args, long long) == 9000000000LL;
    ok &= va_arg(args, int) == -123;
    ok &= va_arg(args, uint64_t) == UINT64_C(18000000000000000000);
    ok &= va_arg(args, uint32_t) == UINT32_C(4000000000);
    ok &= va_arg(args, int) == 255;
    ok &= va_arg(args, int) == 1;
    ok &= va_arg(args, int) == (int)(char)0xff;
    ok &= va_arg(args, double) == (double)(float)1.1;
    ok &= va_arg(args, double) == 2.5;
    ok &= strcmp(va_arg(args, char *), "tail") == 0;
    va_end(args);
    return ok;
}

static long long integer = 42;
static char character = 'Z';
static unsigned char raw = 255;
char high_char(void) { return (char)0xff; }
long long *int_address(void) { return &integer; }
char *char_address(void) { return &character; }
void *void_address(void) { return &raw; }
long long *null_address(void) { return NULL; }

bool check_pointers(int32_t tag, ...) {
    va_list args;
    va_start(args, tag);
    bool ok = tag == 2;
    ok &= va_arg(args, long long *) == &integer;
    ok &= va_arg(args, char *) == &character;
    ok &= va_arg(args, void *) == &raw;
    ok &= va_arg(args, long long *) == NULL;
    va_end(args);
    return ok;
}
