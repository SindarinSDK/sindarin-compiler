/* C helper for test_native_ref_self_assign.sn
 *
 * Reproduces issue #46: `bias = f(bias)` frees bias when f returns the
 * same pointer. Tracks live allocations in a small pool so the test can
 * detect a stale handle deterministically (independent of heap layout). */

#include <stdio.h>
#include <stdlib.h>

#define MAX_TENSORS 64
static __sn__Tensor *g_pool[MAX_TENSORS];
static int g_pool_count = 0;

static void pool_register(__sn__Tensor *t) {
    if (g_pool_count >= MAX_TENSORS) {
        fprintf(stderr, "FAIL: tensor pool overflow\n");
        exit(1);
    }
    g_pool[g_pool_count++] = t;
}

static int pool_contains(__sn__Tensor *t) {
    for (int i = 0; i < g_pool_count; i++) {
        if (g_pool[i] == t) return 1;
    }
    return 0;
}

__sn__Tensor *tensor_create(long long handle) {
    __sn__Tensor *t = calloc(1, sizeof(__sn__Tensor));
    if (!t) { fprintf(stderr, "tensor_create: alloc failed\n"); exit(1); }
    t->__rc__ = 1;
    t->__sn__handle = handle;
    pool_register(t);
    return t;
}

/* Returns the SAME pointer it received. This mirrors the
 * sn_graph_param pattern from the issue: a "configure-and-return-self"
 * builder that does not allocate. */
__sn__Tensor *tensor_register(__sn__Tensor *t) {
    if (!t) { fprintf(stderr, "FAIL: tensor_register: NULL\n"); exit(1); }
    return t;
}

long long tensor_get_handle(__sn__Tensor *t) {
    if (!t) { fprintf(stderr, "FAIL: tensor_get_handle: NULL\n"); exit(1); }
    return t->__sn__handle;
}

long long tensor_check_alive(__sn__Tensor *t) {
    if (!t) { fprintf(stderr, "FAIL: tensor_check_alive: NULL\n"); exit(1); }
    if (!pool_contains(t)) {
        fprintf(stderr, "FAIL: tensor_check_alive: stale handle (not in pool)\n");
        exit(1);
    }
    return 1;
}
