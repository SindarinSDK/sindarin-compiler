/*
 * Arena V2 - GC Cascade False-Positive Tests
 * ============================================
 *
 * Proves that gc_free_dead_handle_children incorrectly kills live handles
 * when a dead handle's data coincidentally contains a live handle's address
 * and that live handle has refcount 0 (only referenced from the stack).
 *
 * The GC builds refcounts by scanning live handle DATA for pointer-sized
 * values matching known handle addresses. A handle only held in a local
 * variable (not embedded in any other handle's data) has refcount 0.
 *
 * gc_free_dead_handle_children then scans dead handle DATA the same way.
 * If a dead handle's data happens to contain a pointer-sized value matching
 * a live handle with refcount 0, the cascade incorrectly marks it DEAD.
 *
 * In production, this manifests as use-after-free / SIGSEGV when a
 * function's local handle variable (e.g. pathParts in matchPath) is
 * collected out from under it because a recently-freed handle's stale
 * memory contains a coincidental address match.
 */

#include "../arena_v2.h"
#include "../arena_gc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("  %s... ", #name); \
    fflush(stdout); \
    test_##name(); \
    printf("PASS\n"); \
    tests_passed++; \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAIL\n    Assertion failed: %s\n    at %s:%d\n", \
               #cond, __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

/* ============================================================================
 * Test: Dead handle data containing live handle address kills the live handle
 * ============================================================================
 *
 * Setup:
 *   1. Allocate "victim" handle on root arena (only held in local variable)
 *   2. Allocate "decoy" handle whose data contains victim's address
 *   3. Mark decoy as DEAD
 *   4. Run GC
 *
 * Expected (correct behavior): victim should still be alive
 * Actual (buggy behavior): victim gets killed by cascade
 *
 * Why: gc_free_dead_handle_children scans decoy's data, finds victim's
 * address, checks refcount (0 — no live handle data contains victim's
 * address, only the stack does), and marks victim DEAD.
 */
TEST(cascade_kills_live_handle)
{
    RtArenaV2 *root = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "root");
    ASSERT(root != NULL);

    /* Step 1: Allocate the victim — a live handle only referenced from stack.
     * Use a non-trivial size so it's clearly a real allocation. */
    RtHandleV2 *victim = rt_arena_v2_strdup(root, "I am alive on the stack");
    ASSERT(victim != NULL);
    ASSERT(!(victim->flags & RT_HANDLE_FLAG_DEAD));

    /* Step 2: Allocate decoy handle with enough room for pointer-sized slots.
     * We'll plant the victim's handle address in the decoy's data. */
    RtHandleV2 *decoy = rt_arena_v2_alloc(root, sizeof(void *) * 4);
    ASSERT(decoy != NULL);

    /* Plant victim's address in decoy's data at a pointer-aligned slot.
     * This simulates what happens in practice when freed memory contains
     * stale pointer values that coincidentally match live handle addresses. */
    void **decoy_data = (void **)decoy->ptr;
    decoy_data[0] = NULL;
    decoy_data[1] = (void *)victim;  /* <-- the false positive trigger */
    decoy_data[2] = NULL;
    decoy_data[3] = NULL;

    /* Step 3: Mark decoy as DEAD. GC will process it. */
    rt_arena_v2_free(decoy);
    ASSERT(decoy->flags & RT_HANDLE_FLAG_DEAD);

    /* Victim is still alive and NOT referenced by any other handle's data.
     * It's only in our local variable — so refcount will be 0. */
    ASSERT(!(victim->flags & RT_HANDLE_FLAG_DEAD));

    /* Step 4: Run GC. The buggy gc_free_dead_handle_children will:
     *   - Scan decoy's data (it's dead)
     *   - Find victim's address at slot [1]
     *   - Check handle_set: victim IS a known handle → match
     *   - Check rctable: victim has refcount 0 (no live handle data refs it)
     *   - Conclude: "exclusively owned by dead handle" → mark DEAD
     *
     * This is WRONG. Victim is live, just stack-referenced. */
    rt_arena_v2_gc(root);

    /* THE BUG: victim should still be alive, but cascade killed it.
     * With correct code, this assertion passes.
     * With buggy code, victim->flags has RT_HANDLE_FLAG_DEAD set. */
    if (victim->flags & RT_HANDLE_FLAG_DEAD) {
        printf("FAIL\n");
        printf("    BUG REPRODUCED: live handle killed by cascade false positive!\n");
        printf("    victim=%p, victim->flags=0x%x (DEAD=%d)\n",
               (void *)victim, victim->flags,
               !!(victim->flags & RT_HANDLE_FLAG_DEAD));
        printf("    victim->ptr=%p (data may already be freed!)\n", victim->ptr);
        tests_failed++;

        /* Clean up carefully — victim may have freed data */
        pthread_mutex_destroy(&root->mutex);
        free(root);
        return;
    }

    /* If we get here, the bug is fixed */
    ASSERT(!(victim->flags & RT_HANDLE_FLAG_DEAD));
    ASSERT(victim->ptr != NULL);
    ASSERT(strcmp((char *)victim->ptr, "I am alive on the stack") == 0);

    /* Cleanup: free remaining handles, then destroy root directly */
    rt_arena_v2_free(victim);
    rt_arena_v2_gc(root);
    pthread_mutex_destroy(&root->mutex);
    free(root);
}

/* ============================================================================
 * Test: Multiple live handles vulnerable to cascade
 * ============================================================================
 *
 * A more realistic scenario: several live handles on the stack, a dead
 * struct-like handle whose data contains multiple handle addresses.
 */
TEST(cascade_kills_multiple_live_handles)
{
    RtArenaV2 *root = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "root");
    ASSERT(root != NULL);

    /* Allocate 3 live handles (simulating local variables in a function) */
    RtHandleV2 *method = rt_arena_v2_strdup(root, "GET");
    RtHandleV2 *path = rt_arena_v2_strdup(root, "/api/items");
    RtHandleV2 *body = rt_arena_v2_strdup(root, "{\"name\":\"test\"}");
    ASSERT(method != NULL && path != NULL && body != NULL);

    /* Allocate a "struct-like" handle that happens to contain addresses
     * of the above handles (simulating a dead HttpRequest whose fields
     * pointed to these strings). */
    size_t struct_size = sizeof(void *) * 8;
    RtHandleV2 *dead_request = rt_arena_v2_alloc(root, struct_size);
    ASSERT(dead_request != NULL);

    void **fields = (void **)dead_request->ptr;
    memset(fields, 0, struct_size);
    fields[0] = (void *)method;  /* request.method */
    fields[2] = (void *)path;    /* request.path */
    fields[4] = (void *)body;    /* request.body */

    /* Kill the struct */
    rt_arena_v2_free(dead_request);

    /* All 3 string handles are live, stack-only, refcount 0 */
    ASSERT(!(method->flags & RT_HANDLE_FLAG_DEAD));
    ASSERT(!(path->flags & RT_HANDLE_FLAG_DEAD));
    ASSERT(!(body->flags & RT_HANDLE_FLAG_DEAD));

    /* Run GC — buggy cascade will kill all 3 */
    rt_arena_v2_gc(root);

    int killed = 0;
    if (method->flags & RT_HANDLE_FLAG_DEAD) killed++;
    if (path->flags & RT_HANDLE_FLAG_DEAD) killed++;
    if (body->flags & RT_HANDLE_FLAG_DEAD) killed++;

    if (killed > 0) {
        printf("FAIL\n");
        printf("    BUG REPRODUCED: %d/3 live handles killed by cascade!\n", killed);
        printf("    method: %s, path: %s, body: %s\n",
               (method->flags & RT_HANDLE_FLAG_DEAD) ? "DEAD" : "alive",
               (path->flags & RT_HANDLE_FLAG_DEAD) ? "DEAD" : "alive",
               (body->flags & RT_HANDLE_FLAG_DEAD) ? "DEAD" : "alive");
        tests_failed++;
        pthread_mutex_destroy(&root->mutex);
        free(root);
        return;
    }

    /* Verify data integrity */
    ASSERT(strcmp((char *)method->ptr, "GET") == 0);
    ASSERT(strcmp((char *)path->ptr, "/api/items") == 0);
    ASSERT(strcmp((char *)body->ptr, "{\"name\":\"test\"}") == 0);

    /* Cleanup */
    rt_arena_v2_free(method);
    rt_arena_v2_free(path);
    rt_arena_v2_free(body);
    rt_arena_v2_gc(root);
    pthread_mutex_destroy(&root->mutex);
    free(root);
}

/* ============================================================================
 * Test: Cross-arena cascade — dead handle in child, live handle in parent
 * ============================================================================
 *
 * Simulates the HTTP server pattern: a child arena (function scope) has a
 * dead handle whose data references a live handle on the parent arena.
 */
TEST(cross_arena_cascade)
{
    RtArenaV2 *root = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "root");
    RtArenaV2 *parent = rt_arena_v2_create(root, RT_ARENA_MODE_DEFAULT, "server");
    RtArenaV2 *child = rt_arena_v2_create(parent, RT_ARENA_MODE_DEFAULT, "handler");
    ASSERT(root != NULL && parent != NULL && child != NULL);

    /* Live handle on parent arena (e.g., a Route's pattern string) */
    RtHandleV2 *route_pattern = rt_arena_v2_strdup(parent, "/items/:id");
    ASSERT(route_pattern != NULL);

    /* Dead handle on child arena whose data contains route_pattern's address
     * (e.g., a temporary struct that held a reference during pattern matching) */
    RtHandleV2 *temp = rt_arena_v2_alloc(child, sizeof(void *) * 4);
    ASSERT(temp != NULL);
    void **temp_data = (void **)temp->ptr;
    temp_data[0] = (void *)route_pattern;
    temp_data[1] = NULL;
    temp_data[2] = NULL;
    temp_data[3] = NULL;

    rt_arena_v2_free(temp);

    /* route_pattern is live, only on stack, refcount 0 */
    ASSERT(!(route_pattern->flags & RT_HANDLE_FLAG_DEAD));

    /* GC walks entire tree — will find temp (dead, in child) and scan its data */
    rt_arena_v2_gc(root);

    if (route_pattern->flags & RT_HANDLE_FLAG_DEAD) {
        printf("FAIL\n");
        printf("    BUG REPRODUCED: cross-arena cascade killed parent's live handle!\n");
        tests_failed++;
        pthread_mutex_destroy(&child->mutex);
        pthread_mutex_destroy(&parent->mutex);
        pthread_mutex_destroy(&root->mutex);
        free(child);
        free(parent);
        free(root);
        return;
    }

    ASSERT(strcmp((char *)route_pattern->ptr, "/items/:id") == 0);

    /* Cleanup: condemn children, GC from root, then free root */
    rt_arena_v2_condemn(child);
    rt_arena_v2_condemn(parent);
    rt_arena_v2_gc(root);
    pthread_mutex_destroy(&root->mutex);
    free(root);
}

/* ============================================================================
 * Test: Cascade should NOT affect handles referenced by live handle data
 * ============================================================================
 *
 * Sanity check: if a live handle's data references another handle, that
 * handle should have refcount >= 1 and be safe from cascade. This should
 * pass even with the buggy code.
 */
TEST(refcounted_handle_survives_cascade)
{
    RtArenaV2 *root = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "root");
    ASSERT(root != NULL);

    /* "inner" handle — will be referenced by live_struct's data */
    RtHandleV2 *inner = rt_arena_v2_strdup(root, "referenced by live data");
    ASSERT(inner != NULL);

    /* live_struct's data contains inner's address → inner gets refcount 1 */
    RtHandleV2 *live_struct = rt_arena_v2_alloc(root, sizeof(void *) * 2);
    ASSERT(live_struct != NULL);
    void **live_data = (void **)live_struct->ptr;
    live_data[0] = (void *)inner;
    live_data[1] = NULL;

    /* Dead handle also contains inner's address */
    RtHandleV2 *dead = rt_arena_v2_alloc(root, sizeof(void *) * 2);
    ASSERT(dead != NULL);
    void **dead_data = (void **)dead->ptr;
    dead_data[0] = (void *)inner;
    dead_data[1] = NULL;
    rt_arena_v2_free(dead);

    rt_arena_v2_gc(root);

    /* inner has refcount 1 (from live_struct), so cascade should spare it */
    ASSERT(!(inner->flags & RT_HANDLE_FLAG_DEAD));
    ASSERT(strcmp((char *)inner->ptr, "referenced by live data") == 0);

    /* live_struct should also be fine */
    ASSERT(!(live_struct->flags & RT_HANDLE_FLAG_DEAD));

    /* Cleanup: free remaining handles, then destroy root directly.
     * Can't use condemn+gc pattern on root (no parent to GC it). */
    rt_arena_v2_free(inner);
    rt_arena_v2_free(live_struct);
    rt_arena_v2_gc(root);
    pthread_mutex_destroy(&root->mutex);
    free(root);
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void)
{
    printf("Arena V2 GC Cascade Tests\n");
    printf("=========================\n\n");

    RUN_TEST(cascade_kills_live_handle);
    RUN_TEST(cascade_kills_multiple_live_handles);
    RUN_TEST(cross_arena_cascade);
    RUN_TEST(refcounted_handle_survives_cascade);

    printf("\n%d/%d tests passed\n", tests_passed, tests_passed + tests_failed);

    if (tests_failed > 0) {
        printf("\n*** %d test(s) demonstrate the GC cascade false-positive bug ***\n",
               tests_failed);
    }

    return tests_failed > 0 ? 1 : 0;
}
