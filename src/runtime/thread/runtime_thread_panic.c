// src/runtime/runtime_thread_panic.c
// Thread Panic Context Functions

/* ============================================================================
 * Thread Panic Context Functions
 * ============================================================================ */

/* Initialize a panic context for the current thread */
void rt_thread_panic_context_init(RtThreadPanicContext *ctx,
                                   RtThreadResult *result,
                                   RtArenaV2 *arena)
{
    if (ctx == NULL) {
        fprintf(stderr, "rt_thread_panic_context_init: NULL context\n");
        return;
    }

    ctx->is_active = true;
    ctx->result = result;
    ctx->arena = arena;

    /* Set as the current thread's panic context */
    rt_thread_panic_ctx = ctx;
}

/* Clear the panic context for the current thread */
void rt_thread_panic_context_clear(void)
{
    if (rt_thread_panic_ctx != NULL) {
        rt_thread_panic_ctx->is_active = false;
    }
    rt_thread_panic_ctx = NULL;
}

/* Check if the current thread has a panic context installed */
bool rt_thread_has_panic_context(void)
{
    return rt_thread_panic_ctx != NULL && rt_thread_panic_ctx->is_active;
}

/* Trigger a panic in the current thread
 * If a panic context is active, longjmp to the handler.
 * Otherwise, print message and exit(1).
 */
void rt_thread_panic(const char *message)
{
    if (rt_thread_has_panic_context()) {
        /* We're in a thread with a panic handler - capture and longjmp */
        RtThreadPanicContext *ctx = rt_thread_panic_ctx;

        /* Set panic state in the result */
        if (ctx->result != NULL) {
            rt_thread_result_set_panic(ctx->result, message, ctx->arena);
        }

        /* Jump back to the wrapper function's setjmp point */
        longjmp(ctx->jump_buffer, 1);
    } else {
        /* No panic handler - print and exit (main thread behavior) */
        if (message != NULL) {
            fprintf(stderr, "panic: %s\n", message);
        } else {
            fprintf(stderr, "panic: (no message)\n");
        }
        exit(1);
    }
}

/* ============================================================================
 * Thread Arena Context
 * ============================================================================
 * Thread-local arena tracking for closures. Allows closures to use the
 * thread's arena when called from a thread context.
 * ============================================================================ */

RT_THREAD_LOCAL void *rt_current_thread_arena = NULL;

/* Set the current thread's arena */
void rt_set_thread_arena(void *arena)
{
    rt_current_thread_arena = arena;
}

/* Get thread arena if set, otherwise return fallback */
void *rt_get_thread_arena_or(void *fallback)
{
    return rt_current_thread_arena != NULL ? rt_current_thread_arena : fallback;
}
