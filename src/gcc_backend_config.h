#ifndef GCC_BACKEND_CONFIG_H
#define GCC_BACKEND_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

/* Backend type enumeration */
typedef enum {
    BACKEND_GCC,
    BACKEND_CLANG,
    BACKEND_TINYCC,
    BACKEND_MSVC
} BackendType;

/* Detect backend type from compiler name */
BackendType detect_backend(const char *cc);

/* Detect backend type from executable name (sn-gcc, sn-clang, sn-tcc, sn-msvc) */
BackendType detect_backend_from_exe(void);

/* Get library subdirectory for backend */
const char *backend_lib_subdir(BackendType backend);

/* Get backend name for error messages */
const char *backend_name(BackendType backend);

/* Filter flags for TinyCC compatibility.
 * TinyCC doesn't support: -flto, -fsanitize=*, -fno-omit-frame-pointer
 * Returns pointer to filtered flags (may be buf or original flags). */
const char *filter_tinycc_flags(const char *flags, char *buf, size_t buf_size);

/* Platform-specific library name translation.
 * Returns the translated name, or the original if no translation is needed. */
const char *translate_lib_name(const char *lib);

/* Get the SDK root directory */
const char *get_sdk_root(const char *compiler_dir);

#endif /* GCC_BACKEND_CONFIG_H */
