/*
 * Arena Redirect - Malloc Redirection API
 * ========================================
 * Redirect malloc/free/realloc to arena allocation.
 * Thread-local stack allows nested redirection contexts.
 */

#ifndef ARENA_REDIRECT_H
#define ARENA_REDIRECT_H

#include "arena_handle.h"

/* Push arena for malloc redirection (thread-local).
 * All malloc/free/realloc calls on this thread will be
 * redirected to the specified arena until popped. */
void rt_arena_v2_redirect_push(RtArenaV2 *arena);

/* Pop malloc redirection.
 * Restores previous redirect context or disables redirection
 * if this was the last arena on the stack. */
void rt_arena_v2_redirect_pop(void);

/* Get current redirect arena for this thread.
 * Returns NULL if no redirection is active. */
RtArenaV2 *rt_arena_v2_redirect_current(void);

#endif /* ARENA_REDIRECT_H */
