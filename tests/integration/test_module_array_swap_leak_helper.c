/* Helper for test_module_array_swap_leak.sn
 *
 * Tracks the number of live Tag objects via a global counter.
 * The test creates Tags, pushes them into nested struct arrays,
 * swaps the module-level buffer, and verifies all Tags are disposed
 * when the old buffer goes out of scope.
 */

#include <stdio.h>
#include <stdlib.h>

static long long g_alive = 0;

__sn__Tag *tag_create(long long id) {
    __sn__Tag *t = calloc(1, sizeof(__sn__Tag));
    if (!t) { fprintf(stderr, "tag_create: alloc failed\n"); exit(1); }
    t->__sn__id = id;
    t->__rc__ = 1;
    g_alive++;
    return t;
}

void tag_dispose(__sn__Tag *t) {
    (void)t;
    g_alive--;
}

long long tag_alive_count(void) {
    return g_alive;
}
