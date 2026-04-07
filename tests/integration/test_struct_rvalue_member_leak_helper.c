/* Helper for test_struct_rvalue_member_leak.sn (issue #49). */

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

long long tracker_alive_count(void) {
    return g_alive;
}
