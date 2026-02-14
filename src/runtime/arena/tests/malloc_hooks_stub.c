/*
 * Stub implementations for malloc hooks
 * Used for standalone arena tests that don't need actual malloc interception.
 */

#include "../../malloc/runtime_malloc_hooks.h"

static __thread RtMallocHandler *tls_stub_handler = NULL;

void rt_malloc_hooks_set_handler(RtMallocHandler *handler)
{
    tls_stub_handler = handler;
}

void rt_malloc_hooks_clear_handler(void)
{
    tls_stub_handler = NULL;
}

RtMallocHandler *rt_malloc_hooks_get_handler(void)
{
    return tls_stub_handler;
}

void *rt_malloc_hooks_orig_malloc(size_t size)
{
    (void)size;
    return NULL;
}

void rt_malloc_hooks_orig_free(void *ptr)
{
    (void)ptr;
}

void *rt_malloc_hooks_orig_realloc(void *ptr, size_t size)
{
    (void)ptr;
    (void)size;
    return NULL;
}
