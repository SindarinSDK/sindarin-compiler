/*
 * Native C implementation for malloc redirect tests
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "runtime/runtime_arena.h"
#include "runtime/runtime_malloc_redirect.h"

/* ============================================================================
 * Basic redirect tests (pure C)
 * ============================================================================ */

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

    /* Suppress unused warnings */
    (void)ptr1;
    (void)ptr2;

    /* Cleanup */
    rt_arena_destroy(arena1);
    rt_arena_destroy(arena2);
    return true;
}

/* ============================================================================
 * Redirect API exposed to Sindarin
 * ============================================================================ */

/* Push redirect context using the given arena */
bool redirect_push(RtArena *arena)
{
    return rt_malloc_redirect_push(arena, NULL);
}

/* Pop redirect context */
void redirect_pop(void)
{
    rt_malloc_redirect_pop();
}

/* Check if redirect is currently active */
bool redirect_is_active(void)
{
    return rt_malloc_redirect_is_active();
}

/* Check if a pointer was allocated from arena */
bool is_arena_ptr(void *ptr)
{
    return rt_malloc_redirect_is_arena_ptr(ptr);
}

/* Get current redirect depth */
int redirect_depth(void)
{
    return (int)rt_malloc_redirect_depth();
}

/* ============================================================================
 * Native functions with arena parameter
 * ============================================================================ */

/* Create a greeting string - uses arena for allocation */
char *create_greeting(RtArena *arena, const char *name)
{
    /* Calculate total length: "Hello, " + name + "!" */
    size_t name_len = strlen(name);
    size_t total_len = 7 + name_len + 1;  /* "Hello, " + name + "!" */

    /* Allocate buffer in arena */
    char *buf = rt_arena_alloc(arena, total_len + 1);
    if (!buf) return NULL;

    /* Build the greeting */
    strcpy(buf, "Hello, ");
    memcpy(buf + 7, name, name_len);
    buf[7 + name_len] = '!';
    buf[total_len] = '\0';

    return buf;
}

/* Concatenate two strings - uses arena for allocation */
char *concat_strings(RtArena *arena, const char *a, const char *b)
{
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    size_t total = len_a + len_b;

    char *buf = rt_arena_alloc(arena, total + 1);
    if (!buf) return NULL;

    memcpy(buf, a, len_a);
    memcpy(buf + len_a, b, len_b);
    buf[total] = '\0';

    return buf;
}

/* ============================================================================
 * TestBuffer native struct implementation
 * ============================================================================ */

typedef struct {
    char *data;
    int size;
    int capacity;
} TestBuffer;

/* Create a buffer with given capacity - uses malloc (can be redirected) */
TestBuffer *buffer_create(int capacity)
{
    /* Allocate the struct itself */
    TestBuffer *buf = (TestBuffer *)malloc(sizeof(TestBuffer));
    if (!buf) return NULL;

    /* Allocate the data buffer */
    buf->data = (char *)malloc(capacity);
    buf->size = 0;
    buf->capacity = buf->data ? capacity : 0;
    if (buf->data) {
        memset(buf->data, 0, capacity);
    }
    return buf;
}

/* Create a buffer using arena directly (no redirect needed) */
TestBuffer *buffer_create_in_arena(RtArena *arena, int capacity)
{
    /* Allocate the struct in arena */
    TestBuffer *buf = (TestBuffer *)rt_arena_alloc(arena, sizeof(TestBuffer));
    if (!buf) return NULL;

    /* Allocate the data buffer in arena */
    buf->data = (char *)rt_arena_alloc(arena, capacity);
    buf->size = 0;
    buf->capacity = buf->data ? capacity : 0;
    if (buf->data) {
        memset(buf->data, 0, capacity);
    }
    return buf;
}

/* Get buffer size */
int buffer_get_size(TestBuffer *self)
{
    return self->size;
}

/* Get buffer capacity */
int buffer_get_capacity(TestBuffer *self)
{
    return self->capacity;
}

/* Check if buffer struct is from arena */
bool buffer_is_arena_ptr(TestBuffer *self)
{
    return rt_malloc_redirect_is_arena_ptr(self);
}

/* Check if buffer's data is from arena */
bool buffer_data_is_arena_ptr(TestBuffer *self)
{
    return rt_malloc_redirect_is_arena_ptr(self->data);
}

/* Write data to buffer */
bool buffer_write(TestBuffer *self, const char *data)
{
    if (!self->data) return false;

    size_t data_len = strlen(data);
    if (self->size + (int)data_len > self->capacity) {
        return false;  /* Would overflow */
    }

    memcpy(self->data + self->size, data, data_len);
    self->size += (int)data_len;
    return true;
}

/* Read buffer contents as string - uses arena */
char *buffer_read_string(RtArena *arena, TestBuffer *self)
{
    if (!self->data || self->size == 0) {
        char *empty = rt_arena_alloc(arena, 1);
        if (empty) empty[0] = '\0';
        return empty;
    }

    /* Allocate and copy to arena */
    char *buf = rt_arena_alloc(arena, self->size + 1);
    if (!buf) return NULL;

    memcpy(buf, self->data, self->size);
    buf[self->size] = '\0';

    return buf;
}

/* Grow buffer - uses malloc (can be redirected) */
bool buffer_grow(TestBuffer *self, int additional)
{
    if (!self->data) return false;

    int new_capacity = self->capacity + additional;
    char *new_data = (char *)malloc(new_capacity);
    if (!new_data) return false;

    /* Copy existing data */
    memcpy(new_data, self->data, self->size);
    memset(new_data + self->size, 0, new_capacity - self->size);

    /* Note: old data is NOT freed - if using arena, it will be cleaned up with arena */
    self->data = new_data;
    self->capacity = new_capacity;

    return true;
}

/* Check if the grown data is from arena */
bool buffer_grown_data_is_arena_ptr(TestBuffer *self)
{
    return rt_malloc_redirect_is_arena_ptr(self->data);
}
