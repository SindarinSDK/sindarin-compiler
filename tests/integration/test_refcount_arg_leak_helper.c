/* Helper for test_refcount_arg_leak.sn
 *
 * Tracks live Foo instances. foo_create increments g_alive;
 * foo_dispose (called by __sn__Foo_release on rc=0) decrements it.
 * foo_get_x is an instance method — used to exercise
 * consume(foo_create(1).getX()) where the intermediate Foo leaks. */

#include <stdlib.h>
#include <stdio.h>

static long long g_alive = 0;

__sn__Foo *foo_create(long long val) {
    __sn__Foo *f = calloc(1, sizeof(__sn__Foo));
    f->__rc__ = 1;
    f->__sn__x = val;
    g_alive++;
    return f;
}

void foo_dispose(__sn__Foo *f) {
    (void)f;
    g_alive--;
}

long long foo_get_x(__sn__Foo *f) {
    return f ? f->__sn__x : -1;
}

long long foo_alive(void) {
    return g_alive;
}
