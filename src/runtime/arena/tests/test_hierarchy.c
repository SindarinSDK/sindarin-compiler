#include "test_framework.h"

static void test_create_child(void)
{
    RtManagedArena *root = rt_managed_arena_create();

    RtManagedArena *child = rt_managed_arena_create_child(root);
    ASSERT(child != NULL, "child arena should not be NULL");
    ASSERT(child->parent == root, "child parent should be root");
    ASSERT(!child->is_root, "child should not be root");
    ASSERT(root->first_child == child, "root's first_child should be child");

    rt_managed_arena_destroy(root);
}

static void test_child_independent_alloc(void)
{
    RtManagedArena *root = rt_managed_arena_create();
    RtManagedArena *child = rt_managed_arena_create_child(root);

    RtHandle rh = rt_managed_alloc(root, RT_HANDLE_NULL, 64);
    char *rp = (char *)rt_managed_pin(root, rh);
    strcpy(rp, "root-data");
    rt_managed_unpin(root, rh);

    RtHandle ch = rt_managed_alloc(child, RT_HANDLE_NULL, 64);
    char *cp = (char *)rt_managed_pin(child, ch);
    strcpy(cp, "child-data");
    rt_managed_unpin(child, ch);

    ASSERT_EQ(rt_managed_live_count(root), 1, "root has 1 live");
    ASSERT_EQ(rt_managed_live_count(child), 1, "child has 1 live");

    rp = (char *)rt_managed_pin(root, rh);
    ASSERT(strcmp(rp, "root-data") == 0, "root data correct");
    rt_managed_unpin(root, rh);

    cp = (char *)rt_managed_pin(child, ch);
    ASSERT(strcmp(cp, "child-data") == 0, "child data correct");
    rt_managed_unpin(child, ch);

    rt_managed_arena_destroy(root);
}

static void test_destroy_child_marks_dead(void)
{
    RtManagedArena *root = rt_managed_arena_create();
    RtManagedArena *child = rt_managed_arena_create_child(root);

    RtHandle handles[5];
    for (int i = 0; i < 5; i++) {
        handles[i] = rt_managed_alloc(child, RT_HANDLE_NULL, 64);
        char *ptr = (char *)rt_managed_pin(child, handles[i]);
        snprintf(ptr, 64, "child-entry-%d", i);
        rt_managed_unpin(child, handles[i]);
    }
    ASSERT_EQ(rt_managed_live_count(child), 5, "child has 5 live");

    RtHandle rh = rt_managed_alloc(root, RT_HANDLE_NULL, 32);
    char *rp = (char *)rt_managed_pin(root, rh);
    strcpy(rp, "root-survives");
    rt_managed_unpin(root, rh);

    rt_managed_arena_destroy_child(child);

    ASSERT_EQ(rt_managed_live_count(root), 1, "root still has 1 live");
    rp = (char *)rt_managed_pin(root, rh);
    ASSERT(strcmp(rp, "root-survives") == 0, "root data survives child destroy");
    rt_managed_unpin(root, rh);

    ASSERT(root->first_child == NULL, "root should have no children after destroy");

    rt_managed_arena_destroy(root);
}

static void test_multiple_children(void)
{
    RtManagedArena *root = rt_managed_arena_create();

    RtManagedArena *c1 = rt_managed_arena_create_child(root);
    RtManagedArena *c2 = rt_managed_arena_create_child(root);
    RtManagedArena *c3 = rt_managed_arena_create_child(root);

    RtHandle h1 = rt_managed_alloc(c1, RT_HANDLE_NULL, 32);
    RtHandle h2 = rt_managed_alloc(c2, RT_HANDLE_NULL, 32);
    RtHandle h3 = rt_managed_alloc(c3, RT_HANDLE_NULL, 32);

    char *p1 = (char *)rt_managed_pin(c1, h1);
    strcpy(p1, "child-1");
    rt_managed_unpin(c1, h1);

    char *p2 = (char *)rt_managed_pin(c2, h2);
    strcpy(p2, "child-2");
    rt_managed_unpin(c2, h2);

    char *p3 = (char *)rt_managed_pin(c3, h3);
    strcpy(p3, "child-3");
    rt_managed_unpin(c3, h3);

    rt_managed_arena_destroy_child(c2);

    p1 = (char *)rt_managed_pin(c1, h1);
    ASSERT(strcmp(p1, "child-1") == 0, "child-1 survives sibling destroy");
    rt_managed_unpin(c1, h1);

    p3 = (char *)rt_managed_pin(c3, h3);
    ASSERT(strcmp(p3, "child-3") == 0, "child-3 survives sibling destroy");
    rt_managed_unpin(c3, h3);

    rt_managed_arena_destroy(root);
}

static void test_deep_nesting(void)
{
    RtManagedArena *root = rt_managed_arena_create();
    RtManagedArena *level1 = rt_managed_arena_create_child(root);
    RtManagedArena *level2 = rt_managed_arena_create_child(level1);
    RtManagedArena *level3 = rt_managed_arena_create_child(level2);

    RtHandle h0 = rt_managed_alloc(root, RT_HANDLE_NULL, 32);
    RtHandle h1 = rt_managed_alloc(level1, RT_HANDLE_NULL, 32);
    RtHandle h2 = rt_managed_alloc(level2, RT_HANDLE_NULL, 32);
    RtHandle h3 = rt_managed_alloc(level3, RT_HANDLE_NULL, 32);

    char *p = (char *)rt_managed_pin(root, h0);
    strcpy(p, "L0");
    rt_managed_unpin(root, h0);

    p = (char *)rt_managed_pin(level1, h1);
    strcpy(p, "L1");
    rt_managed_unpin(level1, h1);

    p = (char *)rt_managed_pin(level2, h2);
    strcpy(p, "L2");
    rt_managed_unpin(level2, h2);

    p = (char *)rt_managed_pin(level3, h3);
    strcpy(p, "L3");
    rt_managed_unpin(level3, h3);

    rt_managed_arena_destroy_child(level2);

    p = (char *)rt_managed_pin(root, h0);
    ASSERT(strcmp(p, "L0") == 0, "root survives grandchild destroy");
    rt_managed_unpin(root, h0);

    p = (char *)rt_managed_pin(level1, h1);
    ASSERT(strcmp(p, "L1") == 0, "level1 survives child destroy");
    rt_managed_unpin(level1, h1);

    ASSERT(level1->first_child == NULL, "level1 has no children after destroy");

    rt_managed_arena_destroy(root);
}

static void test_shared_mode(void)
{
    RtManagedArena *root = rt_managed_arena_create();

    RtManagedArena *parent_scope = rt_managed_arena_create_child(root);
    RtManagedArena *shared_child = parent_scope; /* Shared = reuse parent */

    RtHandle h1 = rt_managed_alloc(parent_scope, RT_HANDLE_NULL, 64);
    RtHandle h2 = rt_managed_alloc(shared_child, RT_HANDLE_NULL, 64);

    char *p1 = (char *)rt_managed_pin(parent_scope, h1);
    strcpy(p1, "parent-alloc");
    rt_managed_unpin(parent_scope, h1);

    char *p2 = (char *)rt_managed_pin(shared_child, h2);
    strcpy(p2, "shared-alloc");
    rt_managed_unpin(shared_child, h2);

    ASSERT_EQ(rt_managed_live_count(parent_scope), 2, "shared mode: both in same arena");

    p1 = (char *)rt_managed_pin(parent_scope, h1);
    ASSERT(strcmp(p1, "parent-alloc") == 0, "parent alloc survives shared return");
    rt_managed_unpin(parent_scope, h1);

    p2 = (char *)rt_managed_pin(parent_scope, h2);
    ASSERT(strcmp(p2, "shared-alloc") == 0, "shared alloc survives in parent");
    rt_managed_unpin(parent_scope, h2);

    rt_managed_arena_destroy(root);
}

static void test_arena_root(void)
{
    RtManagedArena *root = rt_managed_arena_create();
    RtManagedArena *c1 = rt_managed_arena_create_child(root);
    RtManagedArena *c2 = rt_managed_arena_create_child(c1);

    ASSERT(rt_managed_arena_root(root) == root, "root of root is root");
    ASSERT(rt_managed_arena_root(c1) == root, "root of child is root");
    ASSERT(rt_managed_arena_root(c2) == root, "root of grandchild is root");

    rt_managed_arena_destroy(root);
}

static void test_gc_walks_children(void)
{
    RtManagedArena *root = rt_managed_arena_create();
    RtManagedArena *child = rt_managed_arena_create_child(root);

    RtHandle h = RT_HANDLE_NULL;
    for (int i = 0; i < 20; i++) {
        h = rt_managed_alloc(child, h, 64);
    }
    ASSERT_EQ(rt_managed_dead_count(child), 19, "child has 19 dead entries");

    rt_managed_gc_flush(root);

    size_t dead = rt_managed_dead_count(child);
    ASSERT(dead < 19, "root GC cleaned child's dead entries");

    rt_managed_arena_destroy(root);
}

static void test_child_reassignment_stress(void)
{
    RtManagedArena *root = rt_managed_arena_create();

    RtManagedArena *arenas[5];
    RtHandle handles[5];

    for (int depth = 0; depth < 5; depth++) {
        RtManagedArena *parent = (depth == 0) ? root : arenas[depth - 1];
        arenas[depth] = rt_managed_arena_create_child(parent);

        handles[depth] = RT_HANDLE_NULL;
        for (int i = 0; i < 100; i++) {
            handles[depth] = rt_managed_alloc(arenas[depth], handles[depth], 64);
            char *ptr = (char *)rt_managed_pin(arenas[depth], handles[depth]);
            snprintf(ptr, 64, "depth%d-iter%d", depth, i);
            rt_managed_unpin(arenas[depth], handles[depth]);
        }
    }

    for (int depth = 0; depth < 5; depth++) {
        char expected[64];
        snprintf(expected, sizeof(expected), "depth%d-iter99", depth);
        char *ptr = (char *)rt_managed_pin(arenas[depth], handles[depth]);
        ASSERT(strcmp(ptr, expected) == 0, "nested depth value correct");
        rt_managed_unpin(arenas[depth], handles[depth]);
    }

    for (int depth = 4; depth >= 0; depth--) {
        rt_managed_arena_destroy_child(arenas[depth]);
    }

    rt_managed_arena_destroy(root);
}

/* ============================================================================
 * Test: Promotion (child → parent)
 * ============================================================================ */

static void test_promote_basic(void)
{
    RtManagedArena *root = rt_managed_arena_create();
    RtManagedArena *child = rt_managed_arena_create_child(root);

    /* Allocate in child */
    RtHandle ch = rt_managed_alloc(child, RT_HANDLE_NULL, 64);
    char *cp = (char *)rt_managed_pin(child, ch);
    strcpy(cp, "promote-me");
    rt_managed_unpin(child, ch);

    /* Promote to root */
    RtHandle rh = rt_managed_promote(root, child, ch);
    ASSERT(rh != RT_HANDLE_NULL, "promoted handle should not be null");

    /* Source should be dead */
    ASSERT_EQ(rt_managed_live_count(child), 0, "child has 0 live after promote");
    ASSERT_EQ(rt_managed_dead_count(child), 1, "child has 1 dead after promote");

    /* Dest should have the data */
    ASSERT_EQ(rt_managed_live_count(root), 1, "root has 1 live after promote");
    char *rp = (char *)rt_managed_pin(root, rh);
    ASSERT(strcmp(rp, "promote-me") == 0, "promoted data correct");
    rt_managed_unpin(root, rh);

    /* Destroy child — root's promoted handle survives */
    rt_managed_arena_destroy_child(child);

    rp = (char *)rt_managed_pin(root, rh);
    ASSERT(strcmp(rp, "promote-me") == 0, "promoted data survives child destroy");
    rt_managed_unpin(root, rh);

    rt_managed_arena_destroy(root);
}

static void test_promote_multiple(void)
{
    RtManagedArena *root = rt_managed_arena_create();
    RtManagedArena *child = rt_managed_arena_create_child(root);

    /* Allocate several in child, promote some */
    RtHandle ch1 = rt_managed_alloc(child, RT_HANDLE_NULL, 32);
    RtHandle ch2 = rt_managed_alloc(child, RT_HANDLE_NULL, 32);
    RtHandle ch3 = rt_managed_alloc(child, RT_HANDLE_NULL, 32);

    char *p = (char *)rt_managed_pin(child, ch1);
    strcpy(p, "val-1");
    rt_managed_unpin(child, ch1);

    p = (char *)rt_managed_pin(child, ch2);
    strcpy(p, "val-2");
    rt_managed_unpin(child, ch2);

    p = (char *)rt_managed_pin(child, ch3);
    strcpy(p, "val-3");
    rt_managed_unpin(child, ch3);

    /* Promote 1 and 3, leave 2 in child */
    RtHandle rh1 = rt_managed_promote(root, child, ch1);
    RtHandle rh3 = rt_managed_promote(root, child, ch3);

    ASSERT_EQ(rt_managed_live_count(root), 2, "root has 2 promoted");
    ASSERT_EQ(rt_managed_live_count(child), 1, "child has 1 remaining");

    /* Destroy child — ch2 dies, promoted handles survive */
    rt_managed_arena_destroy_child(child);

    p = (char *)rt_managed_pin(root, rh1);
    ASSERT(strcmp(p, "val-1") == 0, "promoted val-1 survives");
    rt_managed_unpin(root, rh1);

    p = (char *)rt_managed_pin(root, rh3);
    ASSERT(strcmp(p, "val-3") == 0, "promoted val-3 survives");
    rt_managed_unpin(root, rh3);

    rt_managed_arena_destroy(root);
}

static void test_promote_deep(void)
{
    /* Promote across multiple levels: grandchild → child → root */
    RtManagedArena *root = rt_managed_arena_create();
    RtManagedArena *child = rt_managed_arena_create_child(root);
    RtManagedArena *grandchild = rt_managed_arena_create_child(child);

    RtHandle gh = rt_managed_alloc(grandchild, RT_HANDLE_NULL, 64);
    char *gp = (char *)rt_managed_pin(grandchild, gh);
    strcpy(gp, "deep-value");
    rt_managed_unpin(grandchild, gh);

    /* Promote grandchild → child */
    RtHandle ch = rt_managed_promote(child, grandchild, gh);
    ASSERT(ch != RT_HANDLE_NULL, "first promote succeeds");

    /* Promote child → root */
    RtHandle rh = rt_managed_promote(root, child, ch);
    ASSERT(rh != RT_HANDLE_NULL, "second promote succeeds");

    /* Destroy grandchild and child */
    rt_managed_arena_destroy_child(grandchild);
    rt_managed_arena_destroy_child(child);

    /* Root still has the data */
    char *rp = (char *)rt_managed_pin(root, rh);
    ASSERT(strcmp(rp, "deep-value") == 0, "data survives double promote");
    rt_managed_unpin(root, rh);

    rt_managed_arena_destroy(root);
}

static void test_promote_with_reassignment(void)
{
    /* Promote into a global that already has a value (old handle marked dead) */
    RtManagedArena *root = rt_managed_arena_create();
    RtManagedArena *child = rt_managed_arena_create_child(root);

    /* Root has an existing "global" */
    RtHandle global = rt_managed_alloc(root, RT_HANDLE_NULL, 64);
    char *gp = (char *)rt_managed_pin(root, global);
    strcpy(gp, "old-global-value");
    rt_managed_unpin(root, global);

    /* Child creates a new value */
    RtHandle ch = rt_managed_alloc(child, RT_HANDLE_NULL, 64);
    char *cp = (char *)rt_managed_pin(child, ch);
    strcpy(cp, "new-value-from-child");
    rt_managed_unpin(child, ch);

    /* Promote child value, marking old global dead */
    RtHandle new_global = rt_managed_promote(root, child, ch);

    /* Now mark the old global dead (simulates reassignment) */
    pthread_mutex_lock(&root->alloc_mutex);
    RtHandleEntry *old_entry = rt_handle_get(root, global);
    if (!old_entry->dead) {
        old_entry->dead = true;
        atomic_fetch_add(&root->dead_bytes, old_entry->size);
        atomic_fetch_sub(&root->live_bytes, old_entry->size);
    }
    pthread_mutex_unlock(&root->alloc_mutex);

    ASSERT_EQ(rt_managed_live_count(root), 1, "root has 1 live (new global)");
    ASSERT_EQ(rt_managed_dead_count(root), 1, "root has 1 dead (old global)");

    rt_managed_arena_destroy_child(child);

    char *np = (char *)rt_managed_pin(root, new_global);
    ASSERT(strcmp(np, "new-value-from-child") == 0, "promoted value is the new global");
    rt_managed_unpin(root, new_global);

    rt_managed_arena_destroy(root);
}

static void test_promote_null_cases(void)
{
    RtManagedArena *root = rt_managed_arena_create();
    RtManagedArena *child = rt_managed_arena_create_child(root);

    /* Promote null handle */
    RtHandle r = rt_managed_promote(root, child, RT_HANDLE_NULL);
    ASSERT_EQ(r, RT_HANDLE_NULL, "promote null handle returns null");

    /* Promote with null dest */
    RtHandle ch = rt_managed_alloc(child, RT_HANDLE_NULL, 32);
    r = rt_managed_promote(NULL, child, ch);
    ASSERT_EQ(r, RT_HANDLE_NULL, "promote to null dest returns null");

    /* Promote with null src */
    r = rt_managed_promote(root, NULL, ch);
    ASSERT_EQ(r, RT_HANDLE_NULL, "promote from null src returns null");

    /* Promote invalid handle index */
    r = rt_managed_promote(root, child, 99999);
    ASSERT_EQ(r, RT_HANDLE_NULL, "promote invalid handle returns null");

    rt_managed_arena_destroy(root);
}

static void test_promote_stress(void)
{
    RtManagedArena *root = rt_managed_arena_create();

    /* Simulate 100 function calls, each promoting a return value */
    RtHandle global = RT_HANDLE_NULL;

    for (int i = 0; i < 100; i++) {
        RtManagedArena *child = rt_managed_arena_create_child(root);

        /* Function body: allocate and compute result */
        RtHandle local = rt_managed_alloc(child, RT_HANDLE_NULL, 64);
        char *lp = (char *)rt_managed_pin(child, local);
        snprintf(lp, 64, "result-%d", i);
        rt_managed_unpin(child, local);

        /* Return: promote to parent, mark old global dead */
        RtHandle promoted = rt_managed_promote(root, child, local);

        /* Reassign global (mark old dead) */
        if (global != RT_HANDLE_NULL) {
            pthread_mutex_lock(&root->alloc_mutex);
            RtHandleEntry *old = rt_handle_get(root, global);
            if (!old->dead) {
                old->dead = true;
                atomic_fetch_add(&root->dead_bytes, old->size);
                atomic_fetch_sub(&root->live_bytes, old->size);
            }
            pthread_mutex_unlock(&root->alloc_mutex);
        }
        global = promoted;

        /* Function returns — child destroyed */
        rt_managed_arena_destroy_child(child);
    }

    /* Only the last promoted value should be live */
    ASSERT_EQ(rt_managed_live_count(root), 1, "only last promoted value live");

    char *rp = (char *)rt_managed_pin(root, global);
    ASSERT(strcmp(rp, "result-99") == 0, "final promoted value correct");
    rt_managed_unpin(root, global);

    rt_managed_arena_destroy(root);
}

void test_hierarchy_run(void)
{
    printf("\n-- Arena Hierarchy --\n");
    TEST_RUN("create child arena", test_create_child);
    TEST_RUN("child independent allocation", test_child_independent_alloc);
    TEST_RUN("destroy child marks handles dead", test_destroy_child_marks_dead);
    TEST_RUN("multiple children", test_multiple_children);
    TEST_RUN("deep nesting (3 levels)", test_deep_nesting);
    TEST_RUN("shared mode (reuse parent)", test_shared_mode);
    TEST_RUN("rt_managed_arena_root()", test_arena_root);
    TEST_RUN("GC walks child arenas", test_gc_walks_children);
    TEST_RUN("child reassignment stress (5x100)", test_child_reassignment_stress);

    printf("\n-- Promotion (child → parent) --\n");
    TEST_RUN("basic promote", test_promote_basic);
    TEST_RUN("promote multiple values", test_promote_multiple);
    TEST_RUN("promote across levels (deep)", test_promote_deep);
    TEST_RUN("promote with global reassignment", test_promote_with_reassignment);
    TEST_RUN("promote null/invalid cases", test_promote_null_cases);
    TEST_RUN("promote stress (100 function calls)", test_promote_stress);
}
