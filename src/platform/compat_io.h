/*
 * I/O compatibility layer for Windows
 * Provides open_memstream and other POSIX I/O functions
 */

#ifndef SN_COMPAT_IO_H
#define SN_COMPAT_IO_H

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

/* ============================================================================
 * open_memstream implementation for Windows (both MSVC and MinGW)
 *
 * This provides a FILE* that writes to a dynamically growing memory buffer.
 * On fclose(), the buffer contains all written data.
 * Note: MinGW doesn't provide open_memstream (it's a glibc extension).
 * ============================================================================ */

/* Memstream state structure */
typedef struct {
    char **bufptr;      /* Pointer to user's buffer pointer */
    size_t *sizeptr;    /* Pointer to user's size variable */
    char *buffer;       /* Internal buffer */
    size_t capacity;    /* Current buffer capacity */
    size_t position;    /* Current write position */
} _memstream_state;

/* Cookie functions for funopen/fopencookie equivalent */
/* On Windows, we use a temporary file approach or a custom solution */

/*
 * Simple implementation using a temporary file that is read back on close.
 * This is less efficient than a true memstream but works reliably.
 */

/* Global registry for memstream tracking (simple linked list) */
typedef struct _memstream_entry {
    FILE *file;
    char *temppath;
    char **bufptr;
    size_t *sizeptr;
    struct _memstream_entry *next;
} _memstream_entry;

/* Note: In a multi-threaded application, this would need synchronization */
static _memstream_entry *_memstream_list = NULL;

/* Register a memstream */
static inline void _memstream_register(FILE *f, const char *path, char **bufptr, size_t *sizeptr) {
    _memstream_entry *entry = (_memstream_entry *)malloc(sizeof(_memstream_entry));
    if (entry) {
        entry->file = f;
#if defined(__MINGW32__) || defined(__MINGW64__)
        entry->temppath = strdup(path);
#else
        entry->temppath = _strdup(path);
#endif
        entry->bufptr = bufptr;
        entry->sizeptr = sizeptr;
        entry->next = _memstream_list;
        _memstream_list = entry;
    }
}

/* Find and finalize a memstream on close */
static inline int _memstream_finalize(FILE *f) {
    _memstream_entry **pp = &_memstream_list;
    while (*pp) {
        if ((*pp)->file == f) {
            _memstream_entry *entry = *pp;
            *pp = entry->next;

            /* Get file size */
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            if (size < 0) size = 0;

            /* Read content into buffer */
            char *buf = (char *)malloc(size + 1);
            if (buf) {
                fseek(f, 0, SEEK_SET);
                size_t read_size = fread(buf, 1, size, f);
                buf[read_size] = '\0';
                *(entry->bufptr) = buf;
                *(entry->sizeptr) = read_size;
            }

            /* Close and delete temp file */
            fclose(f);
            if (entry->temppath) {
#if defined(__MINGW32__) || defined(__MINGW64__)
                unlink(entry->temppath);
#else
                _unlink(entry->temppath);
#endif
                free(entry->temppath);
            }
            free(entry);
            return 0;
        }
        pp = &(*pp)->next;
    }
    return -1; /* Not a memstream, do normal close */
}

/* open_memstream - create a stream that writes to a malloc'd buffer */
static inline FILE *open_memstream(char **bufptr, size_t *sizeptr) {
    if (bufptr == NULL || sizeptr == NULL) {
        return NULL;
    }

    /* Initialize output pointers */
    *bufptr = NULL;
    *sizeptr = 0;

    /* Create a temporary file */
    char temppath[MAX_PATH];
    char tempdir[MAX_PATH];

    if (GetTempPathA(MAX_PATH, tempdir) == 0) {
        return NULL;
    }

    if (GetTempFileNameA(tempdir, "mem", 0, temppath) == 0) {
        return NULL;
    }

    /* Open the temp file for read/write */
    FILE *f = fopen(temppath, "w+b");
    if (f == NULL) {
#if defined(__MINGW32__) || defined(__MINGW64__)
        unlink(temppath);
#else
        _unlink(temppath);
#endif
        return NULL;
    }

    /* Register for later finalization */
    _memstream_register(f, temppath, bufptr, sizeptr);

    return f;
}

/* Custom fclose wrapper that handles memstreams */
/* Note: This requires replacing fclose calls for memstream FILEs */
static inline int fclose_memstream(FILE *f) {
    if (_memstream_finalize(f) == 0) {
        return 0;  /* Was a memstream, already closed */
    }
    return fclose(f);  /* Normal file */
}

/* sn_fclose - cross-platform fclose that handles memstreams on Windows */
/* Use this instead of fclose() for streams that might be from open_memstream */
static inline int sn_fclose(FILE *f) {
    return fclose_memstream(f);
}

/* ============================================================================
 * The following POSIX functions are only needed for MSVC/clang-cl.
 * MinGW provides these natively.
 * ============================================================================ */

#if !defined(__MINGW32__) && !defined(__MINGW64__)

/* ============================================================================
 * dprintf - write formatted output to a file descriptor
 * ============================================================================ */

#include <stdarg.h>

static inline int dprintf(int fd, const char *format, ...) {
    va_list args;
    va_start(args, format);

    /* Get required buffer size */
    va_list args_copy;
    va_copy(args_copy, args);
    int size = _vscprintf(format, args_copy);
    va_end(args_copy);

    if (size < 0) {
        va_end(args);
        return -1;
    }

    /* Allocate buffer and format */
    char *buffer = (char *)malloc(size + 1);
    if (buffer == NULL) {
        va_end(args);
        return -1;
    }

    int result = vsprintf_s(buffer, size + 1, format, args);
    va_end(args);

    if (result >= 0) {
        result = _write(fd, buffer, result);
    }

    free(buffer);
    return result;
}

/* vdprintf */
static inline int vdprintf(int fd, const char *format, va_list ap) {
    va_list args_copy;
    va_copy(args_copy, ap);
    int size = _vscprintf(format, args_copy);
    va_end(args_copy);

    if (size < 0) {
        return -1;
    }

    char *buffer = (char *)malloc(size + 1);
    if (buffer == NULL) {
        return -1;
    }

    int result = vsprintf_s(buffer, size + 1, format, ap);
    if (result >= 0) {
        result = _write(fd, buffer, result);
    }

    free(buffer);
    return result;
}

/* ============================================================================
 * popen/pclose - pipe to/from a process
 * ============================================================================ */

static inline FILE *popen(const char *command, const char *mode) {
    return _popen(command, mode);
}

static inline int pclose(FILE *stream) {
    return _pclose(stream);
}

/* ============================================================================
 * fileno - get file descriptor from FILE*
 * ============================================================================ */

static inline int fileno(FILE *stream) {
    return _fileno(stream);
}

/* ============================================================================
 * fdopen - associate a stream with a file descriptor
 * ============================================================================ */

static inline FILE *fdopen(int fd, const char *mode) {
    return _fdopen(fd, mode);
}

#endif /* !defined(__MINGW32__) && !defined(__MINGW64__) */

#endif /* _WIN32 */

/* ============================================================================
 * Cross-platform helpers (available on all platforms)
 * ============================================================================ */

/* On POSIX systems (not Windows), sn_fclose is just fclose */
#ifndef _WIN32
#include <stdio.h>
static inline int sn_fclose(FILE *f) {
    return fclose(f);
}
#endif

#endif /* SN_COMPAT_IO_H */
