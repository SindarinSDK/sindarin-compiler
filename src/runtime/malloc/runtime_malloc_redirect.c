/*
 * Arena-Redirected Malloc Implementation
 *
 * See runtime_malloc_redirect.h for full documentation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "runtime_malloc_redirect.h"
#include "arena/arena_v2.h"

#ifdef SN_MALLOC_REDIRECT

/* ============================================================================
 * Thread-Local State
 * ============================================================================ */

/* Thread-local redirect state stack (NULL = not redirecting) */
static __thread RtRedirectState *tls_redirect_state = NULL;

/* Thread-local guard to prevent recursive hook calls */
static __thread int tls_hook_guard = 0;

/* Original function pointers - populated by hooking libraries */
static void *(*orig_malloc)(size_t) = NULL;
static void (*orig_free)(void *) = NULL;
static void *(*orig_calloc)(size_t, size_t) = NULL;
static void *(*orig_realloc)(void *, size_t) = NULL;

/* Hook installation state */
static bool hooks_installed = false;

/* Include split modules */
#include "runtime_malloc_redirect_hashset.c"
#include "runtime_malloc_redirect_state.c"
#include "runtime_malloc_redirect_track.c"
#include "runtime_malloc_redirect_alloc.c"
#include "runtime_malloc_redirect_hooks.c"

#endif /* SN_MALLOC_REDIRECT */
