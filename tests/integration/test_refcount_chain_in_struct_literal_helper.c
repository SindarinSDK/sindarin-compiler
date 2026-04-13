/* Helper for test_refcount_chain_in_struct_literal.sn
 *
 * Tracks live Tracker instances via an alive counter.
 * tracker_create increments g_alive; tracker_dispose (called by the
 * compiler-generated __sn__Tracker_release on rc=0) decrements it.
 * tracker_get_value is an instance method that returns the id — the
 * test uses it to exercise Tracker.create(id).getValue() chains
 * inside struct literal field initializers. */

#include <stdio.h>
#include <stdlib.h>

static long long g_alive = 0;

__sn__Tracker *tracker_create(long long id) {
    __sn__Tracker *t = calloc(1, sizeof(__sn__Tracker));
    if (!t) { fprintf(stderr, "tracker_create: alloc failed\n"); exit(1); }
    t->__sn__id = id;
    t->__rc__ = 1;
    g_alive++;
    return t;
}

void tracker_dispose(__sn__Tracker *t) {
    (void)t;
    g_alive--;
}

long long tracker_get_value(__sn__Tracker *t) {
    return t ? t->__sn__id : -1;
}

long long tracker_alive_count(void) {
    return g_alive;
}
