/*
 * dirent.h compatibility layer for Windows
 * Provides POSIX directory iteration API
 */

#ifndef SN_COMPAT_DIRENT_H
#define SN_COMPAT_DIRENT_H

/* Only needed for MSVC/clang-cl on Windows, not MinGW (which provides dirent.h) */
#if defined(_WIN32) && !defined(__MINGW32__) && !defined(__MINGW64__)

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* ============================================================================
 * Directory entry structure
 * ============================================================================ */

#define NAME_MAX 260
#define DT_UNKNOWN 0
#define DT_REG 8
#define DT_DIR 4
#define DT_LNK 10

struct dirent {
    unsigned long d_ino;           /* Inode number (not used on Windows) */
    unsigned char d_type;          /* File type */
    char d_name[MAX_PATH];         /* Filename */
};

/* ============================================================================
 * DIR structure
 * ============================================================================ */

typedef struct {
    HANDLE handle;
    WIN32_FIND_DATAA find_data;
    struct dirent entry;
    int first_read;
} DIR;

/* ============================================================================
 * Directory functions
 * ============================================================================ */

/* opendir - open a directory stream */
static inline DIR *opendir(const char *dirname) {
    if (dirname == NULL || dirname[0] == '\0') {
        errno = ENOENT;
        return NULL;
    }

    /* Allocate DIR structure */
    DIR *dir = (DIR *)malloc(sizeof(DIR));
    if (dir == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    /* Build search pattern (dirname\*) */
    char pattern[MAX_PATH];
    size_t len = strlen(dirname);

    if (len >= MAX_PATH - 3) {
        free(dir);
        errno = ENAMETOOLONG;
        return NULL;
    }

    strcpy(pattern, dirname);

    /* Add backslash if not present */
    if (pattern[len - 1] != '\\' && pattern[len - 1] != '/') {
        pattern[len] = '\\';
        pattern[len + 1] = '\0';
        len++;
    }
    strcat(pattern, "*");

    /* Find first file */
    dir->handle = FindFirstFileA(pattern, &dir->find_data);
    if (dir->handle == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        free(dir);
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
            errno = ENOENT;
        } else if (err == ERROR_ACCESS_DENIED) {
            errno = EACCES;
        } else {
            errno = EIO;
        }
        return NULL;
    }

    dir->first_read = 1;
    return dir;
}

/* readdir - read a directory entry */
static inline struct dirent *readdir(DIR *dir) {
    if (dir == NULL) {
        errno = EBADF;
        return NULL;
    }

    /* First call returns the result from FindFirstFile */
    if (dir->first_read) {
        dir->first_read = 0;
    } else {
        /* Subsequent calls use FindNextFile */
        if (!FindNextFileA(dir->handle, &dir->find_data)) {
            DWORD err = GetLastError();
            if (err == ERROR_NO_MORE_FILES) {
                return NULL;  /* End of directory */
            }
            errno = EIO;
            return NULL;
        }
    }

    /* Fill in dirent structure */
    dir->entry.d_ino = 0;  /* Not available on Windows */
    strncpy(dir->entry.d_name, dir->find_data.cFileName, MAX_PATH - 1);
    dir->entry.d_name[MAX_PATH - 1] = '\0';

    /* Set file type */
    if (dir->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        dir->entry.d_type = DT_DIR;
    } else if (dir->find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        dir->entry.d_type = DT_LNK;
    } else {
        dir->entry.d_type = DT_REG;
    }

    return &dir->entry;
}

/* closedir - close a directory stream */
static inline int closedir(DIR *dir) {
    if (dir == NULL) {
        errno = EBADF;
        return -1;
    }

    if (dir->handle != INVALID_HANDLE_VALUE) {
        FindClose(dir->handle);
    }

    free(dir);
    return 0;
}

/* rewinddir - reset directory stream */
static inline void rewinddir(DIR *dir) {
    if (dir == NULL) return;

    /* Close current handle */
    if (dir->handle != INVALID_HANDLE_VALUE) {
        FindClose(dir->handle);
    }

    /* Note: We can't properly rewind without knowing the original path */
    /* This is a limitation of this simple implementation */
    dir->handle = INVALID_HANDLE_VALUE;
    dir->first_read = 1;
}

#endif /* _WIN32 */

#endif /* SN_COMPAT_DIRENT_H */
