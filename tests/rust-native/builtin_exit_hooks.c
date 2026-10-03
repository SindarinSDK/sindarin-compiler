#include <stdio.h>
#include <stdlib.h>
static void callback_a(void) { fputs("callback-A\n", stdout); fputs("stderr-A\n", stderr); }
static void callback_b(void) { fputs("callback-B\n", stdout); fputs("stderr-B\n", stderr); }
int arm_exit_hooks(void) {
    if (setvbuf(stdout, NULL, _IOFBF, BUFSIZ) != 0) return 1;
    if (setvbuf(stderr, NULL, _IONBF, 0) != 0) return 1;
    int status = atexit(callback_a) + atexit(callback_b);
    fputs("native-before\n", stdout);
    return status;
}
int c_exit_code(const char *raw) { return (int)strtoll(raw, NULL, 10); }

int initialize_exit_hooks(void) {
    if (arm_exit_hooks() != 0) return 0;
    fputs("initializer\n", stdout);
    return 40;
}
