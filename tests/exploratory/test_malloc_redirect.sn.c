/*
 * Native C implementation for malloc redirect tests
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "runtime/runtime_arena.h"
#include "runtime/runtime_malloc_redirect.h"

/* Test basic redirect enable/disable */
bool test_redirect_basic(void)
{
    /* Create an arena for redirected allocations */
    RtArena *arena = rt_arena_create(NULL);
    if (!arena) return false;

    /* Verify redirect is not active initially */
    if (rt_malloc_redirect_is_active()) {
        rt_arena_destroy(arena);
        return false;
    }

    /* Enable redirect */
    if (!rt_malloc_redirect_push(arena, NULL)) {
        rt_arena_destroy(arena);
        return false;
    }

    /* Verify redirect is now active */
    if (!rt_malloc_redirect_is_active()) {
        rt_malloc_redirect_pop();
        rt_arena_destroy(arena);
        return false;
    }

    /* Do a malloc - should be redirected to arena */
    void *ptr = malloc(100);
    if (!ptr) {
        rt_malloc_redirect_pop();
        rt_arena_destroy(arena);
        return false;
    }

    /* Verify the pointer is from the arena */
    if (!rt_malloc_redirect_is_arena_ptr(ptr)) {
        rt_malloc_redirect_pop();
        rt_arena_destroy(arena);
        return false;
    }

    /* Free is a no-op for arena memory (with default policy) */
    free(ptr);

    /* Disable redirect */
    rt_malloc_redirect_pop();

    /* Verify redirect is no longer active */
    if (rt_malloc_redirect_is_active()) {
        rt_arena_destroy(arena);
        return false;
    }

    /* Cleanup - arena destruction frees all redirected memory */
    rt_arena_destroy(arena);
    return true;
}

/* Test nested redirect scopes */
bool test_redirect_nested(void)
{
    RtArena *arena1 = rt_arena_create(NULL);
    RtArena *arena2 = rt_arena_create(NULL);
    if (!arena1 || !arena2) {
        if (arena1) rt_arena_destroy(arena1);
        if (arena2) rt_arena_destroy(arena2);
        return false;
    }

    /* Push first scope */
    if (!rt_malloc_redirect_push(arena1, NULL)) {
        rt_arena_destroy(arena1);
        rt_arena_destroy(arena2);
        return false;
    }

    /* Verify depth is 1 */
    if (rt_malloc_redirect_depth() != 1) {
        rt_malloc_redirect_pop();
        rt_arena_destroy(arena1);
        rt_arena_destroy(arena2);
        return false;
    }

    /* Allocate in first scope */
    void *ptr1 = malloc(50);
    if (!rt_malloc_redirect_is_arena_ptr(ptr1)) {
        rt_malloc_redirect_pop();
        rt_arena_destroy(arena1);
        rt_arena_destroy(arena2);
        return false;
    }

    /* Push second scope (nested) */
    if (!rt_malloc_redirect_push(arena2, NULL)) {
        rt_malloc_redirect_pop();
        rt_arena_destroy(arena1);
        rt_arena_destroy(arena2);
        return false;
    }

    /* Verify depth is 2 */
    if (rt_malloc_redirect_depth() != 2) {
        rt_malloc_redirect_pop();
        rt_malloc_redirect_pop();
        rt_arena_destroy(arena1);
        rt_arena_destroy(arena2);
        return false;
    }

    /* Allocate in second scope */
    void *ptr2 = malloc(75);
    if (!rt_malloc_redirect_is_arena_ptr(ptr2)) {
        rt_malloc_redirect_pop();
        rt_malloc_redirect_pop();
        rt_arena_destroy(arena1);
        rt_arena_destroy(arena2);
        return false;
    }

    /* Pop second scope */
    rt_malloc_redirect_pop();

    /* Verify depth is back to 1 */
    if (rt_malloc_redirect_depth() != 1) {
        rt_malloc_redirect_pop();
        rt_arena_destroy(arena1);
        rt_arena_destroy(arena2);
        return false;
    }

    /* Pop first scope */
    rt_malloc_redirect_pop();

    /* Verify depth is 0 */
    if (rt_malloc_redirect_depth() != 0) {
        rt_arena_destroy(arena1);
        rt_arena_destroy(arena2);
        return false;
    }

    /* Cleanup */
    rt_arena_destroy(arena1);
    rt_arena_destroy(arena2);
    return true;
}

