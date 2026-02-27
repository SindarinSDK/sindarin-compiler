#ifndef RT_ARENA_SAFEPOINT_H
#define RT_ARENA_SAFEPOINT_H

#include <stdbool.h>

/* Global flag — relaxed atomic read in the fast path */
extern _Atomic bool rt_gc_safepoint_requested;

/* Initialize the safepoint subsystem (call once from main) */
void rt_safepoint_init(void);

/* Thread registration (call from thread wrapper entry/exit) */
void rt_safepoint_thread_register(void);
void rt_safepoint_thread_deregister(void);

/* Slow path — park until GC completes */
void rt_safepoint_park(void);

/* GC calls these to stop/resume the world */
void rt_safepoint_request_stw(void);
void rt_safepoint_release_stw(void);

/* Fast-path poll — check if GC wants to stop the world */
void rt_safepoint_poll(void);

/* Query the number of registered threads */
int rt_safepoint_thread_count(void);

/* Native-call transitions — mark thread as "in native code" so GC
 * doesn't wait for it. Call enter_native before blocking native calls
 * (e.g., libuv event loop, syscalls) and leave_native when returning
 * to managed Sindarin code. While in native state, the thread is
 * effectively parked from GC's perspective. */
void rt_safepoint_enter_native(void);
void rt_safepoint_leave_native(void);

#endif
