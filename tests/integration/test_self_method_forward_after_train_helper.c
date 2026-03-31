/* C helper for test_self_method_forward_after_train.sn
 * Simulates a native resource pool (analogous to ggml tensor pool).
 * Tracks allocations to detect use-after-free or stale handles. */

#include <stdio.h>
#include <stdlib.h>

/* ---------- pool tracking ---------- */
#define MAX_RESOURCES 64
static __sn__Resource *g_pool[MAX_RESOURCES];
static int g_pool_count = 0;

static void pool_register(__sn__Resource *r) {
    if (g_pool_count < MAX_RESOURCES)
        g_pool[g_pool_count++] = r;
}

/* ---------- native function implementations ---------- */

__sn__Resource *resource_create(long long id, long long value) {
    __sn__Resource *r = calloc(1, sizeof(__sn__Resource));
    if (!r) { fprintf(stderr, "resource_create: alloc failed\n"); exit(1); }
    r->__rc__ = 1;
    r->__sn__id = id;
    r->__sn__value = value;
    pool_register(r);
    return r;
}

long long resource_get_id(__sn__Resource *r) {
    if (!r) { fprintf(stderr, "FAIL: resource_get_id called on NULL\n"); exit(1); }
    return r->__sn__id;
}

long long resource_get_value(__sn__Resource *r) {
    if (!r) { fprintf(stderr, "FAIL: resource_get_value called on NULL\n"); exit(1); }
    return r->__sn__value;
}

void resource_update(__sn__Resource *r, long long new_value) {
    if (!r) { fprintf(stderr, "FAIL: resource_update called on NULL\n"); exit(1); }
    r->__sn__value = new_value;
}

/* Simulate a "training" operation that processes an array of resources.
 * Analogous to sn_graph_param + sn_graph_train in the ggml case. */
void resource_process_array(SnArray *resources, long long count) {
    for (long long i = 0; i < count; i++) {
        __sn__Resource *r = *(__sn__Resource **)sn_array_get(resources, i);
        if (!r) {
            fprintf(stderr, "FAIL: resource_process_array: element %lld is NULL\n", i);
            exit(1);
        }
        /* Verify handle is still in the pool (not freed/stale) */
        int found = 0;
        for (int j = 0; j < g_pool_count; j++) {
            if (g_pool[j] == r) { found = 1; break; }
        }
        if (!found) {
            fprintf(stderr, "FAIL: resource_process_array: element %lld is stale (not in pool)\n", i);
            exit(1);
        }
    }
}
