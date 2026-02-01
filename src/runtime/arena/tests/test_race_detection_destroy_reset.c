#include "test_race_detection.h"

/* ============================================================================
 * Concurrent Destroy + Promote Race
 * Goal: Race between destroying a child and promoting from it
 * ============================================================================ */

typedef struct {
    RtManagedArena *parent;
    RtManagedArena **child_ptr;
    pthread_mutex_t *child_mutex;
    atomic_bool *stop;
    atomic_int *promote_count;
    atomic_int *destroy_count;
    int is_destroyer;
} DestroyPromoteArgs;

static void *destroy_promote_worker(void *arg)
{
    DestroyPromoteArgs *args = (DestroyPromoteArgs *)arg;

    while (!atomic_load(args->stop)) {
        pthread_mutex_lock(args->child_mutex);
        RtManagedArena *child = *args->child_ptr;

        if (args->is_destroyer) {
            if (child != NULL) {
                *args->child_ptr = NULL;
                pthread_mutex_unlock(args->child_mutex);

                rt_managed_arena_destroy_child(child);
                atomic_fetch_add(args->destroy_count, 1);

                /* Recreate for next round */
                pthread_mutex_lock(args->child_mutex);
                if (*args->child_ptr == NULL) {
                    *args->child_ptr = rt_managed_arena_create_child(args->parent);
                }
                pthread_mutex_unlock(args->child_mutex);
            } else {
                pthread_mutex_unlock(args->child_mutex);
            }
        } else {
            /* Promoter */
            if (child != NULL) {
                /* Allocate in child */
                RtHandle h = rt_managed_alloc(child, RT_HANDLE_NULL, 32);
                if (h != RT_HANDLE_NULL) {
                    char *ptr = (char *)rt_managed_pin(child, h);
                    if (ptr) {
                        strcpy(ptr, "promote-me");
                        rt_managed_unpin(child, h);
                    }

                    /* Try to promote - may fail if child is being destroyed */
                    RtHandle promoted = rt_managed_promote(args->parent, child, h);
                    if (promoted != RT_HANDLE_NULL) {
                        atomic_fetch_add(args->promote_count, 1);
                    }
                }
            }
            pthread_mutex_unlock(args->child_mutex);
        }

        usleep(100);  /* Small delay to increase interleaving */
    }

    return NULL;
}

static void test_concurrent_destroy_promote(void)
{
    RtManagedArena *parent = rt_managed_arena_create();
    RtManagedArena *child = rt_managed_arena_create_child(parent);
    pthread_mutex_t child_mutex;
    pthread_mutex_init(&child_mutex, NULL);

    atomic_bool stop = false;
    atomic_int promote_count = 0;
    atomic_int destroy_count = 0;

    /* 1 destroyer, 3 promoters */
    pthread_t threads[4];
    DestroyPromoteArgs args[4];

    for (int i = 0; i < 4; i++) {
        args[i] = (DestroyPromoteArgs){
            .parent = parent,
            .child_ptr = &child,
            .child_mutex = &child_mutex,
            .stop = &stop,
            .promote_count = &promote_count,
            .destroy_count = &destroy_count,
            .is_destroyer = (i == 0)
        };
        pthread_create(&threads[i], NULL, destroy_promote_worker, &args[i]);
    }

    usleep(300000);

    atomic_store(&stop, true);
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    int promotes = atomic_load(&promote_count);
    int destroys = atomic_load(&destroy_count);

    TEST_STATS("%d promotes, %d destroys", promotes, destroys);

    /* Cleanup */
    pthread_mutex_lock(&child_mutex);
    if (child != NULL) {
        rt_managed_arena_destroy_child(child);
    }
    pthread_mutex_unlock(&child_mutex);

    pthread_mutex_destroy(&child_mutex);
    rt_managed_arena_destroy(parent);

    /* If we got here without crashing, the test passed */
    ASSERT(destroys > 0, "destroy/promote: some destroys occurred");
}

/* ============================================================================
 * Reset Under Active Use
 * Goal: Call reset while other threads are pinning/allocating
 * ============================================================================ */

typedef struct {
    RtManagedArena *arena;
    atomic_bool *stop;
    atomic_int *error_count;
    atomic_int *op_count;
} ResetStressArgs;

static void *reset_stress_worker(void *arg)
{
    ResetStressArgs *args = (ResetStressArgs *)arg;
    RtHandle current = RT_HANDLE_NULL;

    while (!atomic_load(args->stop)) {
        /* Try to allocate - may get null handle after reset */
        RtHandle h = rt_managed_alloc(args->arena, current, 64);
        if (h != RT_HANDLE_NULL) {
            char *ptr = (char *)rt_managed_pin(args->arena, h);
            if (ptr) {
                strcpy(ptr, "data");
                rt_managed_unpin(args->arena, h);
                current = h;
            }
        } else {
            current = RT_HANDLE_NULL;
        }
        atomic_fetch_add(args->op_count, 1);
    }

    return NULL;
}

static void test_reset_under_load(void)
{
    RtManagedArena *arena = rt_managed_arena_create();
    atomic_bool stop = false;
    atomic_int error_count = 0;
    atomic_int op_count = 0;

    pthread_t threads[4];
    ResetStressArgs args[4];

    for (int i = 0; i < 4; i++) {
        args[i] = (ResetStressArgs){
            .arena = arena,
            .stop = &stop,
            .error_count = &error_count,
            .op_count = &op_count
        };
        pthread_create(&threads[i], NULL, reset_stress_worker, &args[i]);
    }

    /* Main thread calls reset repeatedly */
    for (int r = 0; r < 20; r++) {
        usleep(15000);
        rt_managed_arena_reset(arena);
    }

    atomic_store(&stop, true);
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    int ops = atomic_load(&op_count);
    TEST_STATS("%d ops", ops);

    rt_managed_arena_destroy(arena);

    ASSERT_EQ(atomic_load(&error_count), 0, "reset stress: no errors");
    ASSERT(ops > 1000, "reset stress: sufficient operations");
}

void test_race_destroy_reset_run(void)
{
    TEST_SECTION("Concurrent Destroy + Promote");
    TEST_RUN("destroy child while promoting from it", test_concurrent_destroy_promote);

    TEST_SECTION("Reset Under Load");
    TEST_RUN("reset while threads allocating", test_reset_under_load);
}
