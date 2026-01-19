/*
 * Platform detection and configuration header
 * Provides unified platform macros and includes appropriate compatibility headers
 */

#ifndef SN_PLATFORM_H
#define SN_PLATFORM_H

/* Platform detection */
#if defined(_WIN32) || defined(_WIN64)
    #define SN_PLATFORM_WINDOWS 1
    /* MinGW provides POSIX compatibility */
    #if defined(__MINGW32__) || defined(__MINGW64__)
        #define SN_PLATFORM_POSIX 1
        #define SN_PLATFORM_MINGW 1
    #else
        #define SN_PLATFORM_POSIX 0
        #define SN_PLATFORM_MINGW 0
    #endif
#elif defined(__linux__) || defined(__APPLE__) || defined(__unix__)
    #define SN_PLATFORM_WINDOWS 0
    #define SN_PLATFORM_POSIX 1
    #define SN_PLATFORM_MINGW 0
#else
    #error "Unsupported platform"
#endif

/* Compiler detection */
#if defined(_MSC_VER)
    #define SN_COMPILER_MSVC 1
    #define SN_COMPILER_GCC 0
    #define SN_COMPILER_CLANG 0
#elif defined(__clang__)
    #define SN_COMPILER_MSVC 0
    #define SN_COMPILER_GCC 0
    #define SN_COMPILER_CLANG 1
#elif defined(__GNUC__)
    #define SN_COMPILER_MSVC 0
    #define SN_COMPILER_GCC 1
    #define SN_COMPILER_CLANG 0
#else
    #define SN_COMPILER_MSVC 0
    #define SN_COMPILER_GCC 0
    #define SN_COMPILER_CLANG 0
#endif

/* MSVC-specific configuration */
#if SN_COMPILER_MSVC
    /* Disable warnings for POSIX function names */
    #pragma warning(disable: 4996)

    /* Use secure CRT functions */
    #ifndef _CRT_SECURE_NO_WARNINGS
        #define _CRT_SECURE_NO_WARNINGS
    #endif
#endif

/* Include platform-specific compatibility headers */
/* Only include Windows compat layer for MSVC/clang-cl, not MinGW */
#if SN_PLATFORM_WINDOWS && !SN_PLATFORM_MINGW
    #include "compat_windows.h"
#endif

#endif /* SN_PLATFORM_H */
