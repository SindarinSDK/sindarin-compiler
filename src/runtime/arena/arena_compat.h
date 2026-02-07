/*
 * Arena pthread compatibility layer
 *
 * This header provides pthread-compatible types and functions for the arena module.
 * On Windows with MinGW, we can use the native pthread implementation.
 * On Windows with MSVC or when SN_USE_WIN32_THREADS is defined, we use the
 * compat_pthread.h shim that wraps Windows threading primitives.
 */

#ifndef ARENA_COMPAT_H
#define ARENA_COMPAT_H

#if defined(_WIN32) && (!defined(__MINGW32__) && !defined(__MINGW64__) || defined(SN_USE_WIN32_THREADS))
    /* Windows: use full compatibility shim with types and functions */
    #include "platform/compat_pthread.h"
#else
    /* POSIX systems and MinGW: use native pthreads */
    #include <pthread.h>
#endif

#endif /* ARENA_COMPAT_H */
