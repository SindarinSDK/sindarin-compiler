#include "gcc_backend_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifdef _WIN32
#include "platform/platform.h"
#include <windows.h>
#include <io.h>
#define access _access
#define R_OK 4
#define SN_PATH_SEP '\\'
#define SN_PATH_SEP_STR "\\"
#else
#include <unistd.h>
#define SN_PATH_SEP '/'
#define SN_PATH_SEP_STR "/"
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

/* MinGW needs sn_get_executable_path */
#if defined(_WIN32) && (defined(__MINGW32__) || defined(__MINGW64__))
static inline ssize_t sn_get_executable_path(char *buf, size_t size) {
    DWORD len = GetModuleFileNameA(NULL, buf, (DWORD)size);
    if (len == 0 || len >= size) {
        return -1;
    }
    return (ssize_t)len;
}
#endif

/* Static buffer for resolved SDK root path */
static char sdk_root_buf[PATH_MAX];

/* Helper: check if file exists and is readable */
static bool file_exists(const char *path)
{
    return access(path, R_OK) == 0;
}

/* Helper: check if directory exists */
#ifndef _WIN32
#include <sys/stat.h>
#endif

static bool dir_exists(const char *path)
{
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path);
    return (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
#endif
}

/* Helper: Find last path separator in string (cross-platform) */
static char *find_last_path_sep(char *path)
{
    char *last = NULL;
    for (char *p = path; *p; p++)
    {
#ifdef _WIN32
        if (*p == '/' || *p == '\\') last = p;
#else
        if (*p == '/') last = p;
#endif
    }
    return last;
}

BackendType detect_backend(const char *cc)
{
    /* Check for tcc/tinycc first (before checking for 'cc' substring) */
    if (strstr(cc, "tcc") != NULL || strstr(cc, "tinycc") != NULL)
    {
        return BACKEND_TINYCC;
    }

    /* Check for clang BEFORE cl to avoid matching "clang" as "cl"ang */
    if (strstr(cc, "clang") != NULL)
    {
        return BACKEND_CLANG;
    }

    /* Check for MSVC (cl.exe) - must be after clang check */
    if (strstr(cc, "cl") != NULL || strstr(cc, "msvc") != NULL)
    {
        return BACKEND_MSVC;
    }

    /* Default to gcc for gcc, cc, or unknown */
    return BACKEND_GCC;
}

const char *backend_lib_subdir(BackendType backend)
{
    switch (backend)
    {
#ifdef _WIN32
        /* On Windows, each compiler may need its own object files */
        case BACKEND_CLANG:  return "lib/clang";
        case BACKEND_TINYCC: return "lib/tinycc";
        case BACKEND_MSVC:   return "lib/msvc";
        case BACKEND_GCC:
        default:             return "lib/gcc";
#else
        /* On Unix (Linux/macOS), gcc and clang produce compatible object files */
        case BACKEND_TINYCC: return "lib/tinycc";
        case BACKEND_CLANG:
        case BACKEND_MSVC:
        case BACKEND_GCC:
        default:             return "lib/gcc";
#endif
    }
}

const char *backend_name(BackendType backend)
{
    switch (backend)
    {
        case BACKEND_CLANG:  return "clang";
        case BACKEND_TINYCC: return "tinycc";
        case BACKEND_MSVC:   return "msvc";
        case BACKEND_GCC:
        default:             return "gcc";
    }
}

const char *filter_tinycc_flags(const char *flags, char *buf, size_t buf_size)
{
    if (flags == NULL || flags[0] == '\0')
    {
        return flags;
    }

    char *out = buf;
    char *out_end = buf + buf_size - 1;
    const char *p = flags;

    while (*p && out < out_end)
    {
        /* Skip leading whitespace */
        while (*p == ' ') p++;
        if (!*p) break;

        /* Find end of token */
        const char *start = p;
        while (*p && *p != ' ') p++;
        size_t len = p - start;

        /* Check if this flag should be filtered out */
        if ((len >= 5 && strncmp(start, "-flto", 5) == 0) ||
            (len >= 10 && strncmp(start, "-fsanitize", 10) == 0) ||
            (len >= 23 && strncmp(start, "-fno-omit-frame-pointer", 23) == 0))
        {
            /* Skip this flag */
            continue;
        }

        /* Copy flag to output */
        if (out != buf && out < out_end)
        {
            *out++ = ' ';
        }
        size_t copy_len = len;
        if (out + copy_len > out_end)
        {
            copy_len = out_end - out;
        }
        memcpy(out, start, copy_len);
        out += copy_len;
    }
    *out = '\0';

    return buf;
}

const char *translate_lib_name(const char *lib)
{
#ifdef _WIN32
    /* Windows library name mappings */
    static const struct {
        const char *from;
        const char *to;
    } win_mappings[] = {
        { "z", "zlib" },  /* zlib uses "libzlib.a" on Windows, "libz.a" on Unix */
        { NULL, NULL }
    };

    for (int i = 0; win_mappings[i].from != NULL; i++) {
        if (strcmp(lib, win_mappings[i].from) == 0) {
            return win_mappings[i].to;
        }
    }
#else
    (void)lib;  /* Unused on non-Windows */
#endif
    return lib;
}

BackendType detect_backend_from_exe(void)
{
    char exe_path[PATH_MAX];
    bool got_path = false;

#ifdef _WIN32
    ssize_t len = sn_get_executable_path(exe_path, sizeof(exe_path));
    if (len != -1)
    {
        exe_path[len] = '\0';
        got_path = true;
    }
#elif defined(__APPLE__)
    uint32_t size = sizeof(exe_path);
    if (_NSGetExecutablePath(exe_path, &size) == 0)
    {
        got_path = true;
    }
#else
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1)
    {
        exe_path[len] = '\0';
        got_path = true;
    }
#endif

    if (!got_path)
    {
#ifdef _WIN32
        return BACKEND_CLANG; /* Default to clang (LLVM-MinGW) on Windows */
#else
        return BACKEND_GCC;   /* Default to GCC on Unix */
#endif
    }

    /* Get basename */
    char *base = find_last_path_sep(exe_path);
    if (base) base++; else base = exe_path;

    /* Check for backend suffix */
    if (strstr(base, "sn-tcc") != NULL || strstr(base, "sn-tinycc") != NULL)
    {
        return BACKEND_TINYCC;
    }
    if (strstr(base, "sn-clang") != NULL)
    {
        return BACKEND_CLANG;
    }
    if (strstr(base, "sn-msvc") != NULL)
    {
        return BACKEND_MSVC;
    }
#ifdef _WIN32
    return BACKEND_CLANG; /* Default to clang (LLVM-MinGW) on Windows */
#else
    return BACKEND_GCC;   /* Default to GCC on Unix */
#endif
}

const char *get_sdk_root(const char *compiler_dir)
{
    const char *env_sdk = getenv("SN_SDK");
    if (env_sdk && env_sdk[0] && dir_exists(env_sdk))
    {
        strncpy(sdk_root_buf, env_sdk, sizeof(sdk_root_buf) - 1);
        sdk_root_buf[sizeof(sdk_root_buf) - 1] = '\0';
        return sdk_root_buf;
    }

    /* Check if compiler_dir has the SDK structure (sn.cfg exists) */
    char config_check[PATH_MAX];
    snprintf(config_check, sizeof(config_check), "%s" SN_PATH_SEP_STR "sn.cfg", compiler_dir);
    if (file_exists(config_check))
    {
        return compiler_dir;
    }

    /* On Windows, the binary in bin/ is a copy, not a symlink.
     * Check for the SDK at ../lib/sindarin/ relative to compiler_dir */
    snprintf(sdk_root_buf, sizeof(sdk_root_buf), "%s" SN_PATH_SEP_STR ".." SN_PATH_SEP_STR "lib" SN_PATH_SEP_STR "sindarin", compiler_dir);
    snprintf(config_check, sizeof(config_check), "%s" SN_PATH_SEP_STR "sn.cfg", sdk_root_buf);
    if (file_exists(config_check))
    {
        return sdk_root_buf;
    }

    /* Fall back to compiler directory (dev/portable mode) */
    return compiler_dir;
}
