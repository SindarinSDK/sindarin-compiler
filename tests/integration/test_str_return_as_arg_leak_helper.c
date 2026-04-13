/* Helper for test_str_return_as_arg_leak.sn
 *
 * Replicates the pattern: obj.getString() → wrap() → process()
 * inside a struct literal. The getString return (strdup'd) must be
 * cleaned up by the compiler. If not, ASAN catches the leak. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static long long g_alive = 0;

__sn__Source *source_create(void) {
    __sn__Source *s = calloc(1, sizeof(__sn__Source));
    s->__sn___v = 42;
    s->__rc__ = 1;
    return s;
}

char *source_get_string(__sn__Source *self, char *key) {
    (void)self;
    char buf[64];
    snprintf(buf, sizeof(buf), "value_for_%s", key);
    return strdup(buf);
}

__sn__Wrapper *wrap_string(char *s) {
    __sn__Wrapper *w = calloc(1, sizeof(__sn__Wrapper));
    w->__sn__value = s ? (long long)strlen(s) : 0;
    w->__rc__ = 1;
    g_alive++;
    /* We do NOT free s — the caller (compiler) owns it */
    return w;
}

void wrapper_dispose(__sn__Wrapper *w) {
    (void)w;
    g_alive--;
}

long long process_wrapper(__sn__Wrapper *w) {
    /* Just read the value — do NOT release. The compiler's sn_auto_Wrapper
     * cleanup at the call site handles the release. */
    return w ? w->__sn__value : 0;
}
