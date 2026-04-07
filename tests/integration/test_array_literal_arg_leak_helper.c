/* Helper for test_array_literal_arg_leak.sn (issue #47).
 *
 * Uses the dispose alias mechanism to count live Tracker instances.
 * tracker_create increments g_alive; tracker_dispose (called by the
 * compiler-generated __sn__Tracker_release on rc=0) decrements it. */

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

void takes_trackers(SnArray *arr) {
    long long n = arr ? sn_array_length(arr) : 0;
    printf("takes_trackers: got %lld\n", n);
}
