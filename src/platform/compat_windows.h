/*
 * Windows compatibility layer
 * Provides POSIX-like functions and types for Windows
 *
 * This is the main compatibility header - include this to get all
 * Windows compatibility functionality.
 */

#ifndef SN_COMPAT_WINDOWS_H
#define SN_COMPAT_WINDOWS_H

#ifdef _WIN32

/* Must define WIN32_LEAN_AND_MEAN before including windows.h */
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
    #define NOMINMAX
#endif

/* Include winsock2.h BEFORE windows.h to avoid conflicts
 * winsock2.h must come first as it defines timeval and other types
 * that would otherwise conflict with our compatibility layer */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#include <direct.h>
#include <process.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <errno.h>

/* ============================================================================
 * Path and size limits
 * ============================================================================ */

/* PATH_MAX - maximum path length (POSIX name for Windows MAX_PATH) */
#ifndef PATH_MAX
    #define PATH_MAX MAX_PATH
#endif

/* ============================================================================
 * Type definitions
 * ============================================================================ */

typedef int pid_t;
typedef intptr_t ssize_t;
typedef int mode_t;

/* ============================================================================
 * File and directory operations
 * ============================================================================ */

/* File access mode constants */
#ifndef F_OK
    #define F_OK 0  /* Existence */
#endif
#ifndef R_OK
    #define R_OK 4  /* Read permission */
#endif
#ifndef W_OK
    #define W_OK 2  /* Write permission */
#endif
#ifndef X_OK
    #define X_OK 1  /* Execute permission (not really supported on Windows) */
#endif

/* stat() macros for file type checking */
#ifndef S_ISREG
    #define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif
#ifndef S_ISDIR
    #define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif
#ifndef S_ISLNK
    #define S_ISLNK(m) (0)  /* Windows doesn't have symlinks in the same way */
#endif

/* access() - check file accessibility */
static inline int access(const char *path, int mode) {
    /* On Windows, X_OK is not meaningful, treat as F_OK */
    if (mode == X_OK) mode = F_OK;
    return _access(path, mode);
}

/* POSIX file/directory functions - use UCRT's built-in versions if available,
 * otherwise map to underscore-prefixed versions */
#ifndef unlink
    #define unlink _unlink
#endif

#ifndef rmdir
    #define rmdir _rmdir
#endif

#ifndef getcwd
    #define getcwd _getcwd
#endif

#ifndef chdir
    #define chdir _chdir
#endif

/* mkdir() with mode (mode is ignored on Windows) */
static inline int mkdir_compat(const char *path, mode_t mode) {
    (void)mode;
    return _mkdir(path);
}
#define mkdir(path, mode) mkdir_compat(path, mode)

/* ============================================================================
 * Process operations
 * ============================================================================ */

/* getpid() - get process ID */
static inline pid_t getpid(void) {
    return (pid_t)_getpid();
}

/* ============================================================================
 * Process control stubs
 * Note: fork/exec model doesn't exist on Windows. These are stub implementations
 * that will cause runtime failures. Proper Windows implementation should use
 * CreateProcess instead.
 * ============================================================================ */

/* fork() - not available on Windows, returns -1 */
static inline pid_t fork(void) {
    /* fork() doesn't exist on Windows. Code using fork() needs to be
     * rewritten to use CreateProcess or spawn functions */
    return -1;
}

/* Wait status macros - these decode the status from waitpid */
#ifndef WIFEXITED
    #define WIFEXITED(status)   (((status) & 0xFF) == 0)
    #define WEXITSTATUS(status) (((status) >> 8) & 0xFF)
    #define WIFSIGNALED(status) (((status) & 0x7F) != 0 && ((status) & 0x7F) != 0x7F)
    #define WTERMSIG(status)    ((status) & 0x7F)
    #define WIFSTOPPED(status)  (((status) & 0xFF) == 0x7F)
    #define WSTOPSIG(status)    (((status) >> 8) & 0xFF)
#endif

/* waitpid() - not available on Windows in the same form */
static inline pid_t waitpid(pid_t pid, int *status, int options) {
    (void)pid;
    (void)options;
    if (status) *status = 0;
    /* waitpid doesn't exist on Windows. Need to use WaitForSingleObject
     * with process handles from CreateProcess */
    return -1;
}

/* execvp() - already defined in process.h, no redefinition needed */

/* _exit() - immediate process termination */
static inline void _exit(int status) {
    ExitProcess((UINT)status);
}

/* ============================================================================
 * File descriptor operations
 * ============================================================================ */

/* Standard file descriptors */
#ifndef STDIN_FILENO
    #define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
    #define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
    #define STDERR_FILENO 2
#endif

/* read/write/close for file descriptors
 * Note: Windows CRT already provides read/write/close in io.h via
 * _read/_write/_close macros, so we don't redefine them here.
 */
#ifndef read
    #define read _read
#endif
#ifndef write
    #define write _write
#endif
#ifndef close
    #define close _close
#endif

/* dup/dup2 */
static inline int dup(int fd) {
    return _dup(fd);
}

static inline int dup2(int oldfd, int newfd) {
    return _dup2(oldfd, newfd);
}

/* pipe() - create a pipe */
static inline int pipe(int pipefd[2]) {
    return _pipe(pipefd, 4096, _O_BINARY);
}

/* isatty() - test if fd is a terminal */
static inline int isatty(int fd) {
    return _isatty(fd);
}

/* ============================================================================
 * String operations
 * ============================================================================ */

/* strcasecmp/strncasecmp */
#define strcasecmp _stricmp
#define strncasecmp _strnicmp

/* strdup - already available as _strdup */
#ifndef strdup
    #define strdup _strdup
#endif

/* ============================================================================
 * Time operations
 * ============================================================================ */

/* localtime_r - thread-safe localtime */
static inline struct tm *localtime_r(const time_t *timep, struct tm *result) {
    errno_t err = localtime_s(result, timep);
    return (err == 0) ? result : NULL;
}

/* gmtime_r - thread-safe gmtime */
static inline struct tm *gmtime_r(const time_t *timep, struct tm *result) {
    errno_t err = gmtime_s(result, timep);
    return (err == 0) ? result : NULL;
}

/* ============================================================================
 * Temporary file operations
 * ============================================================================ */

/* mkstemp - create a unique temporary file */
static inline int mkstemp(char *template) {
    size_t len = strlen(template);
    if (len < 6) return -1;

    /* _mktemp_s modifies the template in place */
    if (_mktemp_s(template, len + 1) != 0) {
        return -1;
    }

    /* Open the file with exclusive create */
    int fd;
    errno_t err = _sopen_s(&fd, template,
                           _O_RDWR | _O_CREAT | _O_EXCL | _O_BINARY,
                           _SH_DENYNO, _S_IREAD | _S_IWRITE);
    if (err != 0) {
        return -1;
    }
    return fd;
}

/* ============================================================================
 * Path operations
 * ============================================================================ */

/* Path separator */
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"

/* Get executable path (replaces readlink("/proc/self/exe", ...)) */
static inline ssize_t sn_get_executable_path(char *buf, size_t size) {
    DWORD len = GetModuleFileNameA(NULL, buf, (DWORD)size);
    if (len == 0 || len >= size) {
        return -1;
    }
    return (ssize_t)len;
}

/* Helper: Check if character is a path separator (for use in dirname/basename) */
static inline int sn_is_path_sep(char c) {
    return c == '/' || c == '\\';
}

/* dirname - get directory portion of path (modifies input like POSIX) */
static inline char *dirname(char *path) {
    static char dot[] = ".";
    if (path == NULL || path[0] == '\0') {
        return dot;
    }

    /* Find the end of the path */
    size_t len = strlen(path);

    /* Remove trailing separators */
    while (len > 0 && sn_is_path_sep(path[len - 1])) {
        len--;
    }

    /* Handle root paths like C:\ or \ */
    if (len == 0) {
        path[0] = path[0];  /* Keep first separator */
        path[1] = '\0';
        return path;
    }

    /* Handle Windows drive letter (C:) */
    if (len == 2 && path[1] == ':') {
        path[2] = '.';
        path[3] = '\0';
        return path;
    }

    /* Find the last separator */
    char *last_sep = NULL;
    for (size_t i = 0; i < len; i++) {
        if (sn_is_path_sep(path[i])) {
            last_sep = path + i;
        }
    }

    if (last_sep == NULL) {
        /* No separator found - return "." */
        return dot;
    }

    /* Handle root directory (separator at start) */
    if (last_sep == path) {
        path[1] = '\0';
        return path;
    }

    /* Handle Windows drive root (e.g., C:\) */
    if (last_sep == path + 2 && path[1] == ':') {
        path[3] = '\0';
        return path;
    }

    /* Truncate at the last separator */
    *last_sep = '\0';
    return path;
}

/* basename - get filename portion of path */
static inline char *basename(char *path) {
    static char dot[] = ".";
    if (path == NULL || path[0] == '\0') {
        return dot;
    }

    size_t len = strlen(path);

    /* Remove trailing separators */
    while (len > 0 && sn_is_path_sep(path[len - 1])) {
        path[--len] = '\0';
    }

    if (len == 0) {
        return path;  /* Was all separators */
    }

    /* Find the last separator */
    char *last_sep = NULL;
    for (size_t i = 0; i < len; i++) {
        if (sn_is_path_sep(path[i])) {
            last_sep = path + i;
        }
    }

    if (last_sep == NULL) {
        return path;  /* No separator - whole path is the name */
    }

    return last_sep + 1;
}

/* readlink - not applicable on Windows, return -1 */
static inline ssize_t readlink(const char *path, char *buf, size_t bufsiz) {
    (void)path;
    (void)buf;
    (void)bufsiz;
    /* readlink doesn't exist on Windows - use sn_get_executable_path for /proc/self/exe */
    return -1;
}

/* ============================================================================
 * Environment operations
 * ============================================================================ */

/* setenv - set environment variable */
static inline int setenv(const char *name, const char *value, int overwrite) {
    if (!overwrite) {
        char *existing = getenv(name);
        if (existing != NULL) return 0;
    }
    return _putenv_s(name, value);
}

/* unsetenv - unset environment variable */
static inline int unsetenv(const char *name) {
    return _putenv_s(name, "");
}

/* ============================================================================
 * Memory stream (open_memstream replacement)
 * Uses a temporary file approach
 * ============================================================================ */

/*
 * Note: open_memstream is complex to implement on Windows.
 * A simple approach is to use a temporary file and read it back.
 * For a true in-memory implementation, you'd need a custom FILE* wrapper.
 * This is a placeholder - actual implementation depends on usage.
 */

/* ============================================================================
 * Sleep operations
 * ============================================================================ */

/* sleep - sleep for seconds */
static inline unsigned int sleep(unsigned int seconds) {
    Sleep(seconds * 1000);
    return 0;
}

/* usleep - sleep for microseconds */
static inline int usleep(unsigned int usec) {
    /* Windows Sleep is in milliseconds, minimum 1ms */
    DWORD ms = usec / 1000;
    if (ms == 0 && usec > 0) ms = 1;
    Sleep(ms);
    return 0;
}

/* nanosleep - high precision sleep */
/* Note: struct timespec is defined in time.h on modern MSVC */

static inline int nanosleep(const struct timespec *req, struct timespec *rem) {
    (void)rem;  /* Remaining time not supported */
    if (req == NULL) return -1;

    /* Convert to milliseconds */
    DWORD ms = (DWORD)(req->tv_sec * 1000 + req->tv_nsec / 1000000);
    if (ms == 0 && (req->tv_sec > 0 || req->tv_nsec > 0)) ms = 1;
    Sleep(ms);
    return 0;
}

/* ============================================================================
 * Include other compatibility headers
 * ============================================================================ */

#include "compat_time.h"
#include "compat_pthread.h"
#include "compat_dirent.h"
#include "compat_io.h"
/* Note: compat_net.h should be included separately when needed */

#endif /* _WIN32 */

#endif /* SN_COMPAT_WINDOWS_H */
