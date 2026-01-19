/*
 * Time compatibility layer for Windows
 * Provides sys/time.h functionality on Windows
 */

#ifndef SN_COMPAT_TIME_H
#define SN_COMPAT_TIME_H

/* Only needed for MSVC/clang-cl on Windows, not MinGW (which is POSIX-compatible) */
#if defined(_WIN32) && !defined(__MINGW32__) && !defined(__MINGW64__)

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <time.h>
#include <stdint.h>

/* ============================================================================
 * timeval structure (from sys/time.h)
 * Note: winsock2.h also defines timeval, so we guard against redefinition
 * ============================================================================ */

/* Only define if not already defined by winsock or other headers */
#if !defined(_WINSOCK2API_) && !defined(_TIMEVAL_DEFINED)
#define _TIMEVAL_DEFINED
struct timeval {
    long tv_sec;   /* seconds */
    long tv_usec;  /* microseconds */
};
#endif

/* ============================================================================
 * timezone structure
 * ============================================================================ */

struct timezone {
    int tz_minuteswest;  /* minutes west of Greenwich */
    int tz_dsttime;      /* type of DST correction */
};

/* ============================================================================
 * gettimeofday - get current time
 * ============================================================================ */

static inline int gettimeofday(struct timeval *tv, struct timezone *tz) {
    if (tv != NULL) {
        /* Get time as 100-nanosecond intervals since January 1, 1601 */
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);

        /* Convert to 64-bit value */
        uint64_t time = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;

        /* Convert to Unix epoch (January 1, 1970) */
        /* Difference between 1601 and 1970 in 100-nanosecond intervals */
        static const uint64_t EPOCH_DIFF = 116444736000000000ULL;
        time -= EPOCH_DIFF;

        /* Convert to seconds and microseconds */
        tv->tv_sec = (long)(time / 10000000ULL);
        tv->tv_usec = (long)((time % 10000000ULL) / 10);
    }

    if (tz != NULL) {
        TIME_ZONE_INFORMATION tzi;
        GetTimeZoneInformation(&tzi);
        tz->tz_minuteswest = tzi.Bias;
        tz->tz_dsttime = 0;  /* DST info not directly available */
    }

    return 0;
}

/* ============================================================================
 * timeradd, timersub - timer arithmetic macros
 * ============================================================================ */

#ifndef timeradd
#define timeradd(a, b, result)                       \
    do {                                             \
        (result)->tv_sec = (a)->tv_sec + (b)->tv_sec; \
        (result)->tv_usec = (a)->tv_usec + (b)->tv_usec; \
        if ((result)->tv_usec >= 1000000) {          \
            ++(result)->tv_sec;                      \
            (result)->tv_usec -= 1000000;            \
        }                                            \
    } while (0)
#endif

#ifndef timersub
#define timersub(a, b, result)                       \
    do {                                             \
        (result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
        (result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
        if ((result)->tv_usec < 0) {                 \
            --(result)->tv_sec;                      \
            (result)->tv_usec += 1000000;            \
        }                                            \
    } while (0)
#endif

/* Note: timercmp and timerclear may be defined by winsock2.h */
#if !defined(_WINSOCK2API_)
#ifndef timercmp
#define timercmp(a, b, CMP)                          \
    (((a)->tv_sec == (b)->tv_sec) ?                  \
     ((a)->tv_usec CMP (b)->tv_usec) :               \
     ((a)->tv_sec CMP (b)->tv_sec))
#endif

#ifndef timerclear
#define timerclear(tvp) ((tvp)->tv_sec = (tvp)->tv_usec = 0)
#endif
#endif /* !_WINSOCK2API_ */

#ifndef timerisset
#define timerisset(tvp) ((tvp)->tv_sec || (tvp)->tv_usec)
#endif

/* ============================================================================
 * High-resolution timing
 * ============================================================================ */

/* Get monotonic time in nanoseconds (for performance measurement) */
static inline uint64_t sn_get_monotonic_ns(void) {
    static LARGE_INTEGER frequency = {0};
    if (frequency.QuadPart == 0) {
        QueryPerformanceFrequency(&frequency);
    }

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    /* Convert to nanoseconds */
    return (uint64_t)(counter.QuadPart * 1000000000ULL / frequency.QuadPart);
}

/* Get monotonic time in microseconds */
static inline uint64_t sn_get_monotonic_us(void) {
    return sn_get_monotonic_ns() / 1000;
}

/* Get monotonic time in milliseconds */
static inline uint64_t sn_get_monotonic_ms(void) {
    return sn_get_monotonic_ns() / 1000000;
}

#endif /* _WIN32 */

#endif /* SN_COMPAT_TIME_H */
