#include "gcc_backend.h"
#include "code_gen.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "platform/platform.h"
    #if defined(__MINGW32__) || defined(__MINGW64__)
    /* MinGW provides most POSIX functions but not fork/wait */
    #include <unistd.h>
    #include <libgen.h>
    #endif
#else
#include <unistd.h>
#include <sys/wait.h>
#include <libgen.h>
#endif
#include <limits.h>

/* macOS needs mach-o/dyld.h for _NSGetExecutablePath */
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

/* MinGW needs sn_get_executable_path (provided by compat_windows.h for MSVC) */
#if defined(_WIN32) && (defined(__MINGW32__) || defined(__MINGW64__))
#include <windows.h>
static inline ssize_t sn_get_executable_path(char *buf, size_t size) {
    DWORD len = GetModuleFileNameA(NULL, buf, (DWORD)size);
    if (len == 0 || len >= size) {
        return -1;
    }
    return (ssize_t)len;
}
#endif

/* Cross-platform path separator */
#ifdef _WIN32
#define SN_PATH_SEP '\\'
#define SN_PATH_SEP_STR "\\"
/* Normalize path separators to backslashes for Windows cmd.exe compatibility */
static void normalize_path_separators(char *path)
{
    for (char *p = path; *p; p++) {
        if (*p == '/') *p = '\\';
    }
}
#else
#define SN_PATH_SEP '/'
#define SN_PATH_SEP_STR "/"
#define normalize_path_separators(path) ((void)0)
#endif

/* Static buffer for compiler directory path */
static char compiler_dir_buf[PATH_MAX];

/* Static buffer for resolved config file path */
static char config_file_buf[PATH_MAX];

/* Static buffer for resolved lib directory path */
static char lib_dir_buf[PATH_MAX];

/* Static buffer for resolved SDK directory path */
static char sdk_dir_buf[PATH_MAX];

/* Static buffer for resolved SDK file path */
static char sdk_file_buf[PATH_MAX];

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

/* Backend type enumeration */
typedef enum {
    BACKEND_GCC,
    BACKEND_CLANG,
    BACKEND_TINYCC,
    BACKEND_MSVC
} BackendType;

/* Detect backend type from compiler name */
static BackendType detect_backend(const char *cc)
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

/* Get library subdirectory for backend */
static const char *backend_lib_subdir(BackendType backend)
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
        /* On Unix (Linux/macOS), gcc and clang produce compatible object files,
         * so we use lib/gcc for both. TinyCC still needs its own directory. */
        case BACKEND_TINYCC: return "lib/tinycc";
        case BACKEND_CLANG:
        case BACKEND_MSVC:
        case BACKEND_GCC:
        default:             return "lib/gcc";
#endif
    }
}

/* Get backend name for error messages */
static const char *backend_name(BackendType backend)
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

/* Filter flags for TinyCC compatibility.
 * TinyCC doesn't support: -flto, -fsanitize=*, -fno-omit-frame-pointer
 * Returns pointer to filtered flags (may be buf or original flags).
 */
static const char *filter_tinycc_flags(const char *flags, char *buf, size_t buf_size)
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

/* Default values for backend configuration */
#define DEFAULT_STD "c99"
#ifdef __APPLE__
/* macOS: ASAN has issues with sigaltstack, so disable it but keep debug symbols */
#define DEFAULT_DEBUG_CFLAGS_GCC "-fno-omit-frame-pointer -g"
#define DEFAULT_DEBUG_CFLAGS_CLANG "-fno-omit-frame-pointer -g"
#else
#define DEFAULT_DEBUG_CFLAGS_GCC "-no-pie -fsanitize=address -fno-omit-frame-pointer -g"
#define DEFAULT_DEBUG_CFLAGS_CLANG "-fsanitize=address -fno-omit-frame-pointer -g"
#endif
#define DEFAULT_RELEASE_CFLAGS_GCC "-O3 -flto"
#define DEFAULT_RELEASE_CFLAGS_CLANG "-O3 -flto"
#define DEFAULT_DEBUG_CFLAGS_TCC "-g"
#define DEFAULT_RELEASE_CFLAGS_TCC "-O2"
#define DEFAULT_DEBUG_CFLAGS_MSVC "/Zi /Od"
#define DEFAULT_RELEASE_CFLAGS_MSVC "/O2 /DNDEBUG"
#define DEFAULT_CFLAGS_MSVC "/W3 /D_CRT_SECURE_NO_WARNINGS"
#define DEFAULT_LDLIBS_MSVC "ws2_32.lib bcrypt.lib"
#define DEFAULT_LDLIBS_CLANG_WIN "-lws2_32 -lbcrypt -lpthread"
#define DEFAULT_LDLIBS_GCC_WIN "-lws2_32 -lbcrypt -lpthread"

/* Static buffers for config file values (persisted for lifetime of process) */
static char cfg_cc[256];
static char cfg_std[64];
static char cfg_debug_cflags[1024];
static char cfg_release_cflags[1024];
static char cfg_cflags[1024];
static char cfg_ldflags[1024];
static char cfg_ldlibs[1024];
static bool cfg_loaded = false;

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

/* Detect backend type from executable name (sn-gcc, sn-clang, sn-tcc, sn-msvc) */
static BackendType detect_backend_from_exe(void)
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

/* Find config file by searching multiple locations.
 * Search order:
 *   1. $SINDARIN_CONFIG environment variable (if set)
 *   2. <exe_dir>/sn.cfg (portable/development mode)
 *   3. Platform-specific system paths
 *   4. Compile-time default (if defined)
 *
 * Returns path to config file or NULL if not found.
 */
static const char *find_config_file(const char *compiler_dir)
{
    /* 1. Check SINDARIN_CONFIG environment variable */
    const char *env_config = getenv("SINDARIN_CONFIG");
    if (env_config && env_config[0] && file_exists(env_config))
    {
        return env_config;
    }

    /* 2. Check next to executable (portable/dev mode) */
    snprintf(config_file_buf, sizeof(config_file_buf), "%s" SN_PATH_SEP_STR "sn.cfg", compiler_dir);
    if (file_exists(config_file_buf))
    {
        return config_file_buf;
    }

    /* 2b. Check FHS-relative: <exe_dir>/../etc/sindarin/sn.cfg */
    snprintf(config_file_buf, sizeof(config_file_buf), "%s" SN_PATH_SEP_STR ".." SN_PATH_SEP_STR "etc" SN_PATH_SEP_STR "sindarin" SN_PATH_SEP_STR "sn.cfg", compiler_dir);
    if (file_exists(config_file_buf))
    {
        return config_file_buf;
    }

#ifdef _WIN32
    /* 3a. Windows: %LOCALAPPDATA%\Sindarin\sn.cfg */
    const char *localappdata = getenv("LOCALAPPDATA");
    if (localappdata && localappdata[0])
    {
        snprintf(config_file_buf, sizeof(config_file_buf), "%s\\Sindarin\\sn.cfg", localappdata);
        if (file_exists(config_file_buf))
        {
            return config_file_buf;
        }
    }

    /* 3b. Windows: %ProgramFiles%\Sindarin\sn.cfg */
    const char *programfiles = getenv("ProgramFiles");
    if (programfiles && programfiles[0])
    {
        snprintf(config_file_buf, sizeof(config_file_buf), "%s\\Sindarin\\sn.cfg", programfiles);
        if (file_exists(config_file_buf))
        {
            return config_file_buf;
        }
    }
#else
    /* 3a. Unix: /etc/sindarin/sn.cfg */
    if (file_exists("/etc/sindarin/sn.cfg"))
    {
        return "/etc/sindarin/sn.cfg";
    }

    /* 3b. Unix: /usr/local/etc/sindarin/sn.cfg */
    if (file_exists("/usr/local/etc/sindarin/sn.cfg"))
    {
        return "/usr/local/etc/sindarin/sn.cfg";
    }

#ifdef __APPLE__
    /* 3c. macOS: /opt/homebrew/etc/sindarin/sn.cfg (Apple Silicon Homebrew) */
    if (file_exists("/opt/homebrew/etc/sindarin/sn.cfg"))
    {
        return "/opt/homebrew/etc/sindarin/sn.cfg";
    }
#endif
#endif

    /* 4. Compile-time default (if defined) */
#ifdef SN_DEFAULT_CONFIG_FILE
    if (file_exists(SN_DEFAULT_CONFIG_FILE))
    {
        return SN_DEFAULT_CONFIG_FILE;
    }
#endif

    return NULL; /* No config file found, use defaults */
}

/* Find runtime library directory by searching multiple locations.
 * Search order:
 *   1. $SINDARIN_LIB environment variable (if set)
 *   2. <exe_dir>/lib/<backend>/ (portable/development mode)
 *   3. <exe_dir>/../lib/sindarin/<backend>/ (FHS-compliant relative)
 *   4. Platform-specific system paths
 *   5. Compile-time default (if defined)
 *
 * Returns path to lib directory or NULL if not found.
 */
static const char *find_lib_dir(const char *compiler_dir, BackendType backend)
{
    const char *backend_subdir = backend_lib_subdir(backend);
    /* backend_lib_subdir returns "lib/gcc", "lib/clang", etc. - extract just the backend name */
    const char *backend_suffix = strrchr(backend_subdir, '/');
    if (backend_suffix) backend_suffix++; else backend_suffix = backend_subdir;

    /* 1. Check SINDARIN_LIB environment variable */
    const char *env_lib = getenv("SINDARIN_LIB");
    if (env_lib && env_lib[0])
    {
        snprintf(lib_dir_buf, sizeof(lib_dir_buf), "%s" SN_PATH_SEP_STR "%s", env_lib, backend_suffix);
        if (dir_exists(lib_dir_buf))
        {
            return lib_dir_buf;
        }
        /* Also try without backend suffix (user may have set full path) */
        if (dir_exists(env_lib))
        {
            strncpy(lib_dir_buf, env_lib, sizeof(lib_dir_buf) - 1);
            lib_dir_buf[sizeof(lib_dir_buf) - 1] = '\0';
            return lib_dir_buf;
        }
    }

    /* 2. Check next to executable (portable/dev mode): <exe_dir>/lib/<backend>/ */
    snprintf(lib_dir_buf, sizeof(lib_dir_buf), "%s" SN_PATH_SEP_STR "%s", compiler_dir, backend_subdir);
    if (dir_exists(lib_dir_buf))
    {
        return lib_dir_buf;
    }

    /* 3. Check FHS-relative: <exe_dir>/../lib/sindarin/<backend>/ */
    snprintf(lib_dir_buf, sizeof(lib_dir_buf), "%s" SN_PATH_SEP_STR ".." SN_PATH_SEP_STR "lib" SN_PATH_SEP_STR "sindarin" SN_PATH_SEP_STR "%s", compiler_dir, backend_suffix);
    if (dir_exists(lib_dir_buf))
    {
        return lib_dir_buf;
    }

#ifdef _WIN32
    /* 4a. Windows: %LOCALAPPDATA%\Sindarin\lib\<backend> */
    const char *localappdata = getenv("LOCALAPPDATA");
    if (localappdata && localappdata[0])
    {
        snprintf(lib_dir_buf, sizeof(lib_dir_buf), "%s\\Sindarin\\lib\\%s", localappdata, backend_suffix);
        if (dir_exists(lib_dir_buf))
        {
            return lib_dir_buf;
        }
    }

    /* 4b. Windows: %ProgramFiles%\Sindarin\lib\<backend> */
    const char *programfiles = getenv("ProgramFiles");
    if (programfiles && programfiles[0])
    {
        snprintf(lib_dir_buf, sizeof(lib_dir_buf), "%s\\Sindarin\\lib\\%s", programfiles, backend_suffix);
        if (dir_exists(lib_dir_buf))
        {
            return lib_dir_buf;
        }
    }
#else
    /* 4a. Unix: /usr/lib/sindarin/<backend> */
    snprintf(lib_dir_buf, sizeof(lib_dir_buf), "/usr/lib/sindarin/%s", backend_suffix);
    if (dir_exists(lib_dir_buf))
    {
        return lib_dir_buf;
    }

    /* 4b. Unix: /usr/local/lib/sindarin/<backend> */
    snprintf(lib_dir_buf, sizeof(lib_dir_buf), "/usr/local/lib/sindarin/%s", backend_suffix);
    if (dir_exists(lib_dir_buf))
    {
        return lib_dir_buf;
    }

#ifdef __APPLE__
    /* 4c. macOS: /opt/homebrew/lib/sindarin/<backend> (Apple Silicon Homebrew) */
    snprintf(lib_dir_buf, sizeof(lib_dir_buf), "/opt/homebrew/lib/sindarin/%s", backend_suffix);
    if (dir_exists(lib_dir_buf))
    {
        return lib_dir_buf;
    }
#endif
#endif

    /* 5. Compile-time default (if defined) */
#ifdef SN_DEFAULT_LIB_DIR
    snprintf(lib_dir_buf, sizeof(lib_dir_buf), SN_DEFAULT_LIB_DIR "/%s", backend_suffix);
    if (dir_exists(lib_dir_buf))
    {
        return lib_dir_buf;
    }
#endif

    return NULL; /* No lib directory found */
}

/* Find SDK directory by searching multiple locations.
 * Search order:
 *   1. $SINDARIN_SDK environment variable (if set)
 *   2. <exe_dir>/sdk/ (portable/development mode)
 *   3. <exe_dir>/../share/sindarin/sdk/ (FHS-compliant relative)
 *   4. Platform-specific system paths
 *   5. Compile-time default (if defined)
 *
 * Returns path to SDK directory or NULL if not found.
 */
static const char *find_sdk_dir(const char *compiler_dir)
{
    /* 1. Check SINDARIN_SDK environment variable */
    const char *env_sdk = getenv("SINDARIN_SDK");
    if (env_sdk && env_sdk[0] && dir_exists(env_sdk))
    {
        strncpy(sdk_dir_buf, env_sdk, sizeof(sdk_dir_buf) - 1);
        sdk_dir_buf[sizeof(sdk_dir_buf) - 1] = '\0';
        return sdk_dir_buf;
    }

    /* 2. Check next to executable (portable/dev mode): <exe_dir>/sdk/ */
    snprintf(sdk_dir_buf, sizeof(sdk_dir_buf), "%s" SN_PATH_SEP_STR "sdk", compiler_dir);
    if (dir_exists(sdk_dir_buf))
    {
        return sdk_dir_buf;
    }

    /* 3. Check FHS-relative: <exe_dir>/../share/sindarin/sdk/ */
    snprintf(sdk_dir_buf, sizeof(sdk_dir_buf), "%s" SN_PATH_SEP_STR ".." SN_PATH_SEP_STR "share" SN_PATH_SEP_STR "sindarin" SN_PATH_SEP_STR "sdk", compiler_dir);
    if (dir_exists(sdk_dir_buf))
    {
        return sdk_dir_buf;
    }

#ifdef _WIN32
    /* 4a. Windows: %LOCALAPPDATA%\Sindarin\sdk */
    const char *localappdata = getenv("LOCALAPPDATA");
    if (localappdata && localappdata[0])
    {
        snprintf(sdk_dir_buf, sizeof(sdk_dir_buf), "%s\\Sindarin\\sdk", localappdata);
        if (dir_exists(sdk_dir_buf))
        {
            return sdk_dir_buf;
        }
    }

    /* 4b. Windows: %ProgramFiles%\Sindarin\sdk */
    const char *programfiles = getenv("ProgramFiles");
    if (programfiles && programfiles[0])
    {
        snprintf(sdk_dir_buf, sizeof(sdk_dir_buf), "%s\\Sindarin\\sdk", programfiles);
        if (dir_exists(sdk_dir_buf))
        {
            return sdk_dir_buf;
        }
    }
#else
    /* 4a. Unix: /usr/share/sindarin/sdk */
    if (dir_exists("/usr/share/sindarin/sdk"))
    {
        return "/usr/share/sindarin/sdk";
    }

    /* 4b. Unix: /usr/local/share/sindarin/sdk */
    if (dir_exists("/usr/local/share/sindarin/sdk"))
    {
        return "/usr/local/share/sindarin/sdk";
    }

#ifdef __APPLE__
    /* 4c. macOS: /opt/homebrew/share/sindarin/sdk (Apple Silicon Homebrew) */
    if (dir_exists("/opt/homebrew/share/sindarin/sdk"))
    {
        return "/opt/homebrew/share/sindarin/sdk";
    }
#endif
#endif

    /* 5. Compile-time default (if defined) */
#ifdef SN_DEFAULT_SDK_DIR
    if (dir_exists(SN_DEFAULT_SDK_DIR))
    {
        return SN_DEFAULT_SDK_DIR;
    }
#endif

    return NULL; /* No SDK directory found */
}

/* Cached SDK directory path (computed once per session) */
static const char *cached_sdk_dir = NULL;
static bool sdk_dir_searched = false;

/* Resolve an SDK import to its full file path.
 * Given a module name (e.g., "math"), returns the full path to the SDK file
 * (e.g., "/usr/share/sindarin/sdk/math.sn") if it exists.
 *
 * Parameters:
 *   compiler_dir - Directory containing the compiler executable
 *   module_name  - Name of the module to import (without .sn extension)
 *
 * Returns path to SDK file or NULL if not found.
 * The returned path is statically allocated - do not free.
 */
const char *gcc_resolve_sdk_import(const char *compiler_dir, const char *module_name)
{
    /* Cache the SDK directory on first call */
    if (!sdk_dir_searched)
    {
        cached_sdk_dir = find_sdk_dir(compiler_dir);
        sdk_dir_searched = true;
    }

    if (!cached_sdk_dir)
    {
        return NULL; /* No SDK directory found */
    }

    /* Construct the full path to the SDK file */
    snprintf(sdk_file_buf, sizeof(sdk_file_buf), "%s" SN_PATH_SEP_STR "%s.sn",
             cached_sdk_dir, module_name);

    if (file_exists(sdk_file_buf))
    {
        return sdk_file_buf;
    }

    return NULL; /* SDK module not found */
}

/* Reset the SDK directory cache (for testing purposes) */
void gcc_reset_sdk_cache(void)
{
    cached_sdk_dir = NULL;
    sdk_dir_searched = false;
}

/* Parse a single line from config file (KEY=VALUE format) */
static void parse_config_line(const char *line)
{
    /* Skip leading whitespace */
    while (*line == ' ' || *line == '\t') line++;

    /* Skip empty lines and comments */
    if (*line == '\0' || *line == '\n' || *line == '#')
        return;

    /* Find the = separator */
    const char *eq = strchr(line, '=');
    if (!eq) return;

    /* Extract key and value */
    size_t key_len = eq - line;
    const char *value = eq + 1;

    /* Trim trailing whitespace/newline from value */
    size_t value_len = strlen(value);
    while (value_len > 0 && (value[value_len-1] == '\n' || value[value_len-1] == '\r' ||
                              value[value_len-1] == ' ' || value[value_len-1] == '\t'))
    {
        value_len--;
    }

    /* Match key and copy value to appropriate buffer */
    if (key_len == 5 && strncmp(line, "SN_CC", 5) == 0)
    {
        if (value_len < sizeof(cfg_cc))
        {
            strncpy(cfg_cc, value, value_len);
            cfg_cc[value_len] = '\0';
        }
    }
    else if (key_len == 6 && strncmp(line, "SN_STD", 6) == 0)
    {
        if (value_len < sizeof(cfg_std))
        {
            strncpy(cfg_std, value, value_len);
            cfg_std[value_len] = '\0';
        }
    }
    else if (key_len == 15 && strncmp(line, "SN_DEBUG_CFLAGS", 15) == 0)
    {
        if (value_len < sizeof(cfg_debug_cflags))
        {
            strncpy(cfg_debug_cflags, value, value_len);
            cfg_debug_cflags[value_len] = '\0';
        }
    }
    else if (key_len == 17 && strncmp(line, "SN_RELEASE_CFLAGS", 17) == 0)
    {
        if (value_len < sizeof(cfg_release_cflags))
        {
            strncpy(cfg_release_cflags, value, value_len);
            cfg_release_cflags[value_len] = '\0';
        }
    }
    else if (key_len == 9 && strncmp(line, "SN_CFLAGS", 9) == 0)
    {
        if (value_len < sizeof(cfg_cflags))
        {
            strncpy(cfg_cflags, value, value_len);
            cfg_cflags[value_len] = '\0';
        }
    }
    else if (key_len == 10 && strncmp(line, "SN_LDFLAGS", 10) == 0)
    {
        if (value_len < sizeof(cfg_ldflags))
        {
            strncpy(cfg_ldflags, value, value_len);
            cfg_ldflags[value_len] = '\0';
        }
    }
    else if (key_len == 9 && strncmp(line, "SN_LDLIBS", 9) == 0)
    {
        if (value_len < sizeof(cfg_ldlibs))
        {
            strncpy(cfg_ldlibs, value, value_len);
            cfg_ldlibs[value_len] = '\0';
        }
    }
}

/* Load config file from compiler directory if it exists */
void cc_backend_load_config(const char *compiler_dir)
{
    if (cfg_loaded) return;
    cfg_loaded = true;

    /* Find config file using multi-path search */
    const char *config_path = find_config_file(compiler_dir);
    if (!config_path)
    {
        return; /* No config file found, use defaults */
    }

    /* Try to open config file */
    FILE *f = fopen(config_path, "r");
    if (!f) return; /* Can't open config file, use defaults */

    /* Parse each line */
    char line[2048];
    while (fgets(line, sizeof(line), f))
    {
        parse_config_line(line);
    }
    fclose(f);
}

void cc_backend_init_config(CCBackendConfig *config)
{
    const char *env_val;

    /* First, determine the actual CC value (priority: env > config > default) */
    const char *actual_cc;
    env_val = getenv("SN_CC");
    if (env_val && env_val[0])
        actual_cc = env_val;
    else if (cfg_cc[0])
        actual_cc = cfg_cc;
    else
        actual_cc = NULL;  /* Will determine from exe name */

    /* Detect backend from actual CC if provided, otherwise from exe name */
    BackendType backend;
    if (actual_cc)
        backend = detect_backend(actual_cc);
    else
        backend = detect_backend_from_exe();

    /* Set backend-specific defaults based on actual backend */
    const char *default_cc;
    const char *default_debug_cflags;
    const char *default_release_cflags;
    const char *default_cflags = "";
    const char *default_ldlibs = "";

    switch (backend)
    {
        case BACKEND_CLANG:
            default_cc = "clang";
            default_debug_cflags = DEFAULT_DEBUG_CFLAGS_CLANG;
            default_release_cflags = DEFAULT_RELEASE_CFLAGS_CLANG;
#ifdef _WIN32
            default_ldlibs = DEFAULT_LDLIBS_CLANG_WIN;
#endif
            break;
        case BACKEND_TINYCC:
            default_cc = "tcc";
            default_debug_cflags = DEFAULT_DEBUG_CFLAGS_TCC;
            default_release_cflags = DEFAULT_RELEASE_CFLAGS_TCC;
            break;
        case BACKEND_MSVC:
            default_cc = "cl";
            default_debug_cflags = DEFAULT_DEBUG_CFLAGS_MSVC;
            default_release_cflags = DEFAULT_RELEASE_CFLAGS_MSVC;
            default_cflags = DEFAULT_CFLAGS_MSVC;
            default_ldlibs = DEFAULT_LDLIBS_MSVC;
            break;
        case BACKEND_GCC:
        default:
            default_cc = "gcc";
            default_debug_cflags = DEFAULT_DEBUG_CFLAGS_GCC;
            default_release_cflags = DEFAULT_RELEASE_CFLAGS_GCC;
#ifdef _WIN32
            default_ldlibs = DEFAULT_LDLIBS_GCC_WIN;
#endif
            break;
    }

    /* Config file values are loaded by cc_backend_load_config() which should be
     * called before this function. If not called, cfg_* buffers remain empty. */

    /* Priority: Environment variable > Config file > Default
     * Config file values are in cfg_* buffers (empty if not set) */

    /* SN_CC: C compiler command (already determined above) */
    if (actual_cc)
        config->cc = actual_cc;
    else
        config->cc = default_cc;

    /* SN_STD: C standard */
    env_val = getenv("SN_STD");
    if (env_val && env_val[0])
        config->std = env_val;
    else if (cfg_std[0])
        config->std = cfg_std;
    else
        config->std = DEFAULT_STD;

    /* SN_DEBUG_CFLAGS: Debug mode flags */
    env_val = getenv("SN_DEBUG_CFLAGS");
    if (env_val && env_val[0])
        config->debug_cflags = env_val;
    else if (cfg_debug_cflags[0])
        config->debug_cflags = cfg_debug_cflags;
    else
        config->debug_cflags = default_debug_cflags;

    /* SN_RELEASE_CFLAGS: Release mode flags */
    env_val = getenv("SN_RELEASE_CFLAGS");
    if (env_val && env_val[0])
        config->release_cflags = env_val;
    else if (cfg_release_cflags[0])
        config->release_cflags = cfg_release_cflags;
    else
        config->release_cflags = default_release_cflags;

    /* SN_CFLAGS: Additional compiler flags */
    env_val = getenv("SN_CFLAGS");
    if (env_val && env_val[0])
        config->cflags = env_val;
    else if (cfg_cflags[0])
        config->cflags = cfg_cflags;
    else
        config->cflags = default_cflags;

    /* SN_LDFLAGS: Additional linker flags (empty by default) */
    env_val = getenv("SN_LDFLAGS");
    if (env_val && env_val[0])
        config->ldflags = env_val;
    else if (cfg_ldflags[0])
        config->ldflags = cfg_ldflags;
    else
        config->ldflags = "";

    /* SN_LDLIBS: Additional libraries */
    env_val = getenv("SN_LDLIBS");
    if (env_val && env_val[0])
        config->ldlibs = env_val;
    else if (cfg_ldlibs[0])
        config->ldlibs = cfg_ldlibs;
    else
        config->ldlibs = default_ldlibs;
}

bool gcc_check_available(const CCBackendConfig *config, bool verbose)
{
    /* Build command to check if compiler is available */
    char check_cmd[PATH_MAX];

    /* MSVC cl.exe doesn't support --version, use different check */
    bool is_msvc = (strcmp(config->cc, "cl") == 0 || strstr(config->cc, "cl.exe") != NULL);

#ifdef _WIN32
    /* Only quote compiler path if it contains spaces (avoid cmd.exe quote parsing issues) */
    const char *cc_quote = (strchr(config->cc, ' ') != NULL) ? "\"" : "";
    if (is_msvc)
    {
        /* cl.exe outputs version to stderr when called with no args, redirect all output */
        snprintf(check_cmd, sizeof(check_cmd), "%s%s%s 2>&1 | findstr /C:\"Microsoft\" > NUL", cc_quote, config->cc, cc_quote);
    }
    else
    {
        snprintf(check_cmd, sizeof(check_cmd), "%s%s%s --version > NUL 2>&1", cc_quote, config->cc, cc_quote);
    }
#else
    snprintf(check_cmd, sizeof(check_cmd), "%s --version > /dev/null 2>&1", config->cc);
#endif

    int result = system(check_cmd);

    if (result == 0)
    {
        if (verbose)
        {
            DEBUG_INFO("C compiler '%s' found and available", config->cc);
        }
        return true;
    }

    fprintf(stderr, "Error: C compiler '%s' is not installed or not in PATH.\n", config->cc);
    if (strcmp(config->cc, "gcc") == 0)
    {
        fprintf(stderr, "To compile Sn programs to executables, please install GCC:\n");
        fprintf(stderr, "  Ubuntu/Debian: sudo apt install gcc\n");
        fprintf(stderr, "  Fedora/RHEL:   sudo dnf install gcc\n");
        fprintf(stderr, "  Arch Linux:    sudo pacman -S gcc\n");
    }
    else if (is_msvc)
    {
        fprintf(stderr, "To use MSVC, run from Visual Studio Developer Command Prompt.\n");
        fprintf(stderr, "Or set SN_CC to a different compiler.\n");
    }
    else
    {
        fprintf(stderr, "Ensure '%s' is installed and in your PATH.\n", config->cc);
        fprintf(stderr, "Or set SN_CC to a different compiler.\n");
    }
    fprintf(stderr, "\nAlternatively, use --emit-c to output C code only.\n");

    return false;
}

bool gcc_validate_pragma_sources(PragmaSourceInfo *source_files, int source_file_count,
                                  bool verbose)
{
#ifdef _WIN32
    (void)source_files;
    (void)source_file_count;
    (void)verbose;
    /* Windows validation not implemented - rely on C compiler errors */
    return true;
#else
    if (source_files == NULL || source_file_count == 0)
        return true;

    bool all_valid = true;

    for (int i = 0; i < source_file_count; i++)
    {
        const char *src = source_files[i].value;
        const char *source_dir = source_files[i].source_dir;

        /* Strip surrounding quotes if present */
        size_t len = strlen(src);
        char unquoted[PATH_MAX];
        if (len >= 2 && src[0] == '"' && src[len - 1] == '"')
        {
            strncpy(unquoted, src + 1, len - 2);
            unquoted[len - 2] = '\0';
            src = unquoted;
        }

        /* Build full path relative to source file directory */
        char full_path[PATH_MAX];
        if (src[0] == '/')
        {
            strncpy(full_path, src, sizeof(full_path) - 1);
            full_path[sizeof(full_path) - 1] = '\0';
        }
        else
        {
            snprintf(full_path, sizeof(full_path), "%s/%s", source_dir, src);
        }

        if (verbose)
        {
            DEBUG_INFO("Checking pragma source: %s", full_path);
        }

        if (!file_exists(full_path))
        {
            fprintf(stderr, "error: pragma source file not found: %s\n", source_files[i].value);
            fprintf(stderr, "  --> Resolved path: %s\n", full_path);
            fprintf(stderr, "  --> Searched relative to: %s\n\n", source_dir);
            all_valid = false;
        }
    }

    return all_valid;
#endif
}

const char *gcc_get_compiler_dir(const char *argv0)
{
#ifdef _WIN32
    /* Windows: use GetModuleFileName via sn_get_executable_path */
    ssize_t len = sn_get_executable_path(compiler_dir_buf, sizeof(compiler_dir_buf));
    if (len != -1)
    {
        /* Resolve symlinks/reparse points (winget uses App Execution Aliases) */
        HANDLE hFile = CreateFileA(compiler_dir_buf, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL, OPEN_EXISTING,
                                   FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            char real_path[PATH_MAX];
            DWORD result = GetFinalPathNameByHandleA(hFile, real_path, sizeof(real_path),
                                                     FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
            CloseHandle(hFile);
            if (result > 0 && result < sizeof(real_path))
            {
                /* GetFinalPathNameByHandle returns \\?\C:\... prefix, skip it */
                const char *path = real_path;
                if (strncmp(path, "\\\\?\\", 4) == 0)
                    path += 4;
                strncpy(compiler_dir_buf, path, sizeof(compiler_dir_buf) - 1);
                compiler_dir_buf[sizeof(compiler_dir_buf) - 1] = '\0';
            }
        }
        /* Get the directory part */
        char *dir = dirname(compiler_dir_buf);
        /* Copy back since dirname may modify in place */
        memmove(compiler_dir_buf, dir, strlen(dir) + 1);
        return compiler_dir_buf;
    }
#elif defined(__APPLE__)
    /* macOS: use _NSGetExecutablePath */
    uint32_t size = sizeof(compiler_dir_buf);
    if (_NSGetExecutablePath(compiler_dir_buf, &size) == 0)
    {
        /* Resolve any symlinks to get the real path */
        char real_path[PATH_MAX];
        if (realpath(compiler_dir_buf, real_path) != NULL)
        {
            strncpy(compiler_dir_buf, real_path, sizeof(compiler_dir_buf) - 1);
            compiler_dir_buf[sizeof(compiler_dir_buf) - 1] = '\0';
        }
        /* Get the directory part */
        char *dir = dirname(compiler_dir_buf);
        /* Copy back since dirname may modify in place */
        memmove(compiler_dir_buf, dir, strlen(dir) + 1);
        return compiler_dir_buf;
    }
#else
    /* Linux: try to resolve via /proc/self/exe */
    ssize_t len = readlink("/proc/self/exe", compiler_dir_buf, sizeof(compiler_dir_buf) - 1);
    if (len != -1)
    {
        compiler_dir_buf[len] = '\0';
        /* Get the directory part */
        char *dir = dirname(compiler_dir_buf);
        /* Copy back since dirname may modify in place */
        memmove(compiler_dir_buf, dir, strlen(dir) + 1);
        return compiler_dir_buf;
    }
#endif

    /* Fallback: use argv[0] */
    if (argv0 != NULL)
    {
        /* Make a copy since dirname may modify the string */
        strncpy(compiler_dir_buf, argv0, sizeof(compiler_dir_buf) - 1);
        compiler_dir_buf[sizeof(compiler_dir_buf) - 1] = '\0';
        char *dir = dirname(compiler_dir_buf);
        memmove(compiler_dir_buf, dir, strlen(dir) + 1);
        return compiler_dir_buf;
    }

    /* Last resort: assume current directory */
    strcpy(compiler_dir_buf, ".");
    return compiler_dir_buf;
}

bool gcc_compile(const CCBackendConfig *config, const char *c_file,
                 const char *output_exe, const char *compiler_dir,
                 bool verbose, bool debug_mode,
                 char **link_libs, int link_lib_count,
                 PragmaSourceInfo *source_files, int source_file_count)
{
    char exe_path[PATH_MAX];
    char lib_dir[PATH_MAX];
    char include_dir[PATH_MAX];
    char deps_include_dir[PATH_MAX];
    char deps_lib_dir[PATH_MAX];
    char runtime_lib[PATH_MAX];
    char command[8192];
    char extra_libs[PATH_MAX];
    char extra_sources[2048];
    char filtered_mode_cflags[1024];
    char c_file_normalized[PATH_MAX];

    /* Copy and normalize the C file path for Windows compatibility */
    strncpy(c_file_normalized, c_file, sizeof(c_file_normalized) - 1);
    c_file_normalized[sizeof(c_file_normalized) - 1] = '\0';
    normalize_path_separators(c_file_normalized);

    /* Detect backend type from compiler name */
    BackendType backend = detect_backend(config->cc);

    /* Find runtime library directory using multi-path search */
    const char *found_lib_dir = find_lib_dir(compiler_dir, backend);
    if (found_lib_dir)
    {
        strncpy(lib_dir, found_lib_dir, sizeof(lib_dir) - 1);
        lib_dir[sizeof(lib_dir) - 1] = '\0';
    }
    else
    {
        /* Fallback to original relative path (for error message) */
        snprintf(lib_dir, sizeof(lib_dir), "%s" SN_PATH_SEP_STR "%s", compiler_dir, backend_lib_subdir(backend));
    }

    /* Build include directory path - also search multiple locations */
    snprintf(include_dir, sizeof(include_dir), "%s" SN_PATH_SEP_STR "include", compiler_dir);
    if (!dir_exists(include_dir))
    {
        /* Try FHS-relative path */
        snprintf(include_dir, sizeof(include_dir), "%s" SN_PATH_SEP_STR ".." SN_PATH_SEP_STR "include" SN_PATH_SEP_STR "sindarin", compiler_dir);
        if (!dir_exists(include_dir))
        {
#ifdef _WIN32
            const char *pf = getenv("ProgramFiles");
            if (pf) snprintf(include_dir, sizeof(include_dir), "%s\\Sindarin\\include", pf);
#else
            /* Try system paths */
            if (dir_exists("/usr/include/sindarin"))
                strcpy(include_dir, "/usr/include/sindarin");
            else if (dir_exists("/usr/local/include/sindarin"))
                strcpy(include_dir, "/usr/local/include/sindarin");
            else
                snprintf(include_dir, sizeof(include_dir), "%s" SN_PATH_SEP_STR "include", compiler_dir);
#endif
        }
    }

    /* Build deps include directory path (for zlib headers) */
    /* Build deps include/lib directory paths (for zlib) */
    /* Deps are at lib_dir/../deps/ (e.g., lib/sindarin/deps/) */
    deps_include_dir[0] = '\0';
    deps_lib_dir[0] = '\0';
    snprintf(deps_include_dir, sizeof(deps_include_dir), "%s" SN_PATH_SEP_STR ".." SN_PATH_SEP_STR "deps" SN_PATH_SEP_STR "include", lib_dir);
    if (dir_exists(deps_include_dir))
    {
        /* Found packaged deps */
        snprintf(deps_lib_dir, sizeof(deps_lib_dir), "%s" SN_PATH_SEP_STR ".." SN_PATH_SEP_STR "deps" SN_PATH_SEP_STR "lib", lib_dir);
    }
    else
    {
        /* Packaged deps not found, clear the path */
        deps_include_dir[0] = '\0';
        /* Try VCPKG_ROOT environment variable first */
        const char *vcpkg_root = getenv("VCPKG_ROOT");
        if (vcpkg_root != NULL && vcpkg_root[0] != '\0')
        {
            /* Try MinGW triplet first (for clang/gcc), then MSVC triplet */
            snprintf(deps_include_dir, sizeof(deps_include_dir), "%s" SN_PATH_SEP_STR "installed" SN_PATH_SEP_STR "x64-mingw-dynamic" SN_PATH_SEP_STR "include", vcpkg_root);
            if (dir_exists(deps_include_dir))
            {
                snprintf(deps_lib_dir, sizeof(deps_lib_dir), "%s" SN_PATH_SEP_STR "installed" SN_PATH_SEP_STR "x64-mingw-dynamic" SN_PATH_SEP_STR "lib", vcpkg_root);
            }
            else
            {
                snprintf(deps_include_dir, sizeof(deps_include_dir), "%s" SN_PATH_SEP_STR "installed" SN_PATH_SEP_STR "x64-windows" SN_PATH_SEP_STR "include", vcpkg_root);
                if (dir_exists(deps_include_dir))
                {
                    snprintf(deps_lib_dir, sizeof(deps_lib_dir), "%s" SN_PATH_SEP_STR "installed" SN_PATH_SEP_STR "x64-windows" SN_PATH_SEP_STR "lib", vcpkg_root);
                }
                else
                {
                    deps_include_dir[0] = '\0';
                }
            }
        }

        /* Try vcpkg path for local development (compiler_dir is bin/, vcpkg is at project root) */
        if (deps_include_dir[0] == '\0')
        {
            snprintf(deps_include_dir, sizeof(deps_include_dir), "%s" SN_PATH_SEP_STR ".." SN_PATH_SEP_STR "vcpkg" SN_PATH_SEP_STR "installed" SN_PATH_SEP_STR "x64-mingw-dynamic" SN_PATH_SEP_STR "include", compiler_dir);
            if (dir_exists(deps_include_dir))
            {
                snprintf(deps_lib_dir, sizeof(deps_lib_dir), "%s" SN_PATH_SEP_STR ".." SN_PATH_SEP_STR "vcpkg" SN_PATH_SEP_STR "installed" SN_PATH_SEP_STR "x64-mingw-dynamic" SN_PATH_SEP_STR "lib", compiler_dir);
            }
            else
            {
                snprintf(deps_include_dir, sizeof(deps_include_dir), "%s" SN_PATH_SEP_STR ".." SN_PATH_SEP_STR "vcpkg" SN_PATH_SEP_STR "installed" SN_PATH_SEP_STR "x64-windows" SN_PATH_SEP_STR "include", compiler_dir);
                if (dir_exists(deps_include_dir))
                {
                    snprintf(deps_lib_dir, sizeof(deps_lib_dir), "%s" SN_PATH_SEP_STR ".." SN_PATH_SEP_STR "vcpkg" SN_PATH_SEP_STR "installed" SN_PATH_SEP_STR "x64-windows" SN_PATH_SEP_STR "lib", compiler_dir);
                }
                else
                {
                    deps_include_dir[0] = '\0';  /* Not found */
                }
            }
        }
    }

    /* Normalize path separators for Windows cmd.exe compatibility */
    normalize_path_separators(lib_dir);
    normalize_path_separators(include_dir);
    if (deps_include_dir[0]) normalize_path_separators(deps_include_dir);
    if (deps_lib_dir[0]) normalize_path_separators(deps_lib_dir);

    if (verbose)
    {
        DEBUG_INFO("Using %s backend, lib_dir=%s", backend_name(backend), lib_dir);
    }

    /* Determine output executable path if not specified */
    if (output_exe == NULL || output_exe[0] == '\0')
    {
        /* Derive from c_file by removing .c extension */
        strncpy(exe_path, c_file, sizeof(exe_path) - 1);
        exe_path[sizeof(exe_path) - 1] = '\0';

        char *dot = strrchr(exe_path, '.');
        if (dot != NULL && strcmp(dot, ".c") == 0)
        {
            *dot = '\0';
        }
        /* else: just use the filename as-is */
    }
    else
    {
        strncpy(exe_path, output_exe, sizeof(exe_path) - 1);
        exe_path[sizeof(exe_path) - 1] = '\0';
    }

    /* Normalize path separators for Windows cmd.exe compatibility */
    normalize_path_separators(exe_path);

    /* Build path to runtime static library */
#ifdef _WIN32
    snprintf(runtime_lib, sizeof(runtime_lib), "%s" SN_PATH_SEP_STR "libsn_runtime.a", lib_dir);
#else
    snprintf(runtime_lib, sizeof(runtime_lib), "%s" SN_PATH_SEP_STR "libsn_runtime.a", lib_dir);
#endif

    /* Check that runtime library exists */
    if (backend != BACKEND_MSVC && access(runtime_lib, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime library not found: %s\n", runtime_lib);
        fprintf(stderr, "The '%s' backend runtime is not built.\n", backend_name(backend));
        fprintf(stderr, "Run 'make build' to build the runtime.\n");
        return false;
    }

    /* Build C compiler command using configuration.
     *
     * Command structure:
     *   $CC $MODE_CFLAGS -w -std=$STD -D_GNU_SOURCE $CFLAGS -I"dir" <sources>
     *        -lpthread -lm $extra_libs $LDLIBS $LDFLAGS -o "output" 2>"errors"
     *
     * We use -w to suppress all warnings on generated code - any issues should
     * be caught by the Sn type checker, not the C compiler. We redirect stderr
     * to capture any errors for display only if compilation actually fails.
     */

    /* Build extra library flags from pragma link directives */
    extra_libs[0] = '\0';
    if (link_libs != NULL && link_lib_count > 0)
    {
        int offset = 0;
        for (int i = 0; i < link_lib_count && offset < (int)sizeof(extra_libs) - 8; i++)
        {
            int written = snprintf(extra_libs + offset, sizeof(extra_libs) - offset, " -l%s", link_libs[i]);
            if (written > 0)
            {
                offset += written;
            }
        }
    }

    /* Build extra source files from pragma source directives.
     * Source files are specified as quoted strings like "helper.c".
     * Paths are resolved relative to each pragma's defining module directory. */
    extra_sources[0] = '\0';
    if (source_files != NULL && source_file_count > 0)
    {
        int offset = 0;
        for (int i = 0; i < source_file_count && offset < (int)sizeof(extra_sources) - 256; i++)
        {
            const char *src = source_files[i].value;
            const char *source_dir = source_files[i].source_dir;

            /* Strip surrounding quotes if present */
            size_t len = strlen(src);
            char unquoted[PATH_MAX];
            if (len >= 2 && src[0] == '"' && src[len - 1] == '"')
            {
                strncpy(unquoted, src + 1, len - 2);
                unquoted[len - 2] = '\0';
                src = unquoted;
            }

            /* Build full path relative to pragma's defining module directory */
            char full_path[PATH_MAX];
            if (src[0] == '/' || (strlen(src) > 1 && src[1] == ':'))
            {
                /* Absolute path - use as is */
                strncpy(full_path, src, sizeof(full_path) - 1);
                full_path[sizeof(full_path) - 1] = '\0';
            }
            else
            {
                /* Relative path - resolve relative to pragma's source directory */
                snprintf(full_path, sizeof(full_path), "%s" SN_PATH_SEP_STR "%s", source_dir, src);
            }
            normalize_path_separators(full_path);

            int written = snprintf(extra_sources + offset, sizeof(extra_sources) - offset, " \"%s\"", full_path);
            if (written > 0)
            {
                offset += written;
            }
        }
    }

    /* Create temporary file for compiler errors */
    char error_file[PATH_MAX];
#ifdef _WIN32
    /* Windows: use TEMP environment variable */
    const char *temp_dir = getenv("TEMP");
    if (!temp_dir) temp_dir = getenv("TMP");
    if (!temp_dir) temp_dir = ".";
    snprintf(error_file, sizeof(error_file), "%s\\sn_cc_errors_%d.txt", temp_dir, (int)getpid());
#else
    /* Unix: use /tmp with mkstemp */
    strcpy(error_file, "/tmp/sn_cc_errors_XXXXXX");
    int error_fd = mkstemp(error_file);
    if (error_fd == -1)
    {
        /* Fallback: use a fixed name */
        strcpy(error_file, "/tmp/sn_cc_errors.txt");
    }
    else
    {
        close(error_fd);
    }
#endif

    /* Select mode-specific flags, filtering for TinyCC if needed */
    const char *mode_cflags = debug_mode ? config->debug_cflags : config->release_cflags;
    if (backend == BACKEND_TINYCC)
    {
        mode_cflags = filter_tinycc_flags(mode_cflags, filtered_mode_cflags, sizeof(filtered_mode_cflags));
    }

    /* Build deps include/lib options (empty if deps not found) */
    char deps_include_opt[PATH_MAX + 8];
    char deps_lib_opt[PATH_MAX + 8];
    if (deps_include_dir[0])
        snprintf(deps_include_opt, sizeof(deps_include_opt), "-I\"%s\"", deps_include_dir);
    else
        deps_include_opt[0] = '\0';
    if (deps_lib_dir[0])
        snprintf(deps_lib_opt, sizeof(deps_lib_opt), "-L\"%s\"", deps_lib_dir);
    else
        deps_lib_opt[0] = '\0';

    /* Build the command - note: config->cflags, config->ldlibs, config->ldflags
     * may be empty strings, which is fine.
     * Use include_dir for headers (-I) and lib_dir for runtime objects.
     */
    if (backend == BACKEND_MSVC)
    {
        /* MSVC uses different command line syntax:
         * - /Fe for output file
         * - /I for include path
         * - Links against sn_runtime.lib instead of individual .o files
         * - No -std, -lpthread, -lm needed
         */
        char runtime_lib[PATH_MAX];
        snprintf(runtime_lib, sizeof(runtime_lib), "%s" SN_PATH_SEP_STR "sn_runtime.lib", lib_dir);

        /* Check that runtime library exists */
        if (access(runtime_lib, R_OK) != 0)
        {
            fprintf(stderr, "Error: Runtime library not found: %s\n", runtime_lib);
            fprintf(stderr, "The MSVC backend runtime is not built.\n");
            fprintf(stderr, "Run CMake build with MSVC to build the runtime library.\n");
            return false;
        }

        /* Only quote compiler path if it contains spaces */
        const char *cc_quote = (strchr(config->cc, ' ') != NULL) ? "\"" : "";
        /* Build MSVC-style deps include option */
        char msvc_deps_opt[PATH_MAX + 8];
        if (deps_include_dir[0])
            snprintf(msvc_deps_opt, sizeof(msvc_deps_opt), "/I\"%s\"", deps_include_dir);
        else
            msvc_deps_opt[0] = '\0';
        snprintf(command, sizeof(command),
            "%s%s%s %s %s /I\"%s\" %s \"%s\"%s \"%s\" %s %s /Fe\"%s\" /link %s 2>\"%s\"",
            cc_quote, config->cc, cc_quote, mode_cflags, config->cflags, include_dir, msvc_deps_opt,
            c_file_normalized, extra_sources, runtime_lib, config->ldlibs, config->ldflags, exe_path,
            config->ldlibs, error_file);
    }
    else
    {
        /* GCC/Clang: Link against static runtime library
         * Using -Wl,--whole-archive ensures all symbols are included even if not
         * directly referenced by the user's code (needed for runtime initialization) */
        const char *cc_quote = (strchr(config->cc, ' ') != NULL) ? "\"" : "";
#ifdef _WIN32
        /* Windows: Use -Wl,--whole-archive to force all symbols from archive */
        snprintf(command, sizeof(command),
            "%s%s%s %s -w -std=%s -DSN_USE_WIN32_THREADS %s -I\"%s\" %s "
            "\"%s\"%s -Wl,--whole-archive \"%s\" -Wl,--no-whole-archive "
            "%s -lpthread -lm%s %s %s -o \"%s\" 2>\"%s\"",
            cc_quote, config->cc, cc_quote, mode_cflags, config->std, config->cflags, include_dir, deps_include_opt,
            c_file_normalized, extra_sources, runtime_lib,
            deps_lib_opt, extra_libs, config->ldlibs, config->ldflags, exe_path, error_file);
#else
        /* Unix: Use -Wl,--whole-archive (Linux) or just link normally (macOS handles it) */
#ifdef __APPLE__
        snprintf(command, sizeof(command),
            "%s%s%s %s -w -std=%s -D_GNU_SOURCE %s -I\"%s\" %s "
            "\"%s\"%s -Wl,-force_load,\"%s\" "
            "%s -lpthread -lm%s %s %s -o \"%s\" 2>\"%s\"",
            cc_quote, config->cc, cc_quote, mode_cflags, config->std, config->cflags, include_dir, deps_include_opt,
            c_file_normalized, extra_sources, runtime_lib,
            deps_lib_opt, extra_libs, config->ldlibs, config->ldflags, exe_path, error_file);
#else
        snprintf(command, sizeof(command),
            "%s%s%s %s -w -std=%s -D_GNU_SOURCE %s -I\"%s\" %s "
            "\"%s\"%s -Wl,--whole-archive \"%s\" -Wl,--no-whole-archive "
            "%s -lpthread -lm%s %s %s -o \"%s\" 2>\"%s\"",
            cc_quote, config->cc, cc_quote, mode_cflags, config->std, config->cflags, include_dir, deps_include_opt,
            c_file_normalized, extra_sources, runtime_lib,
            deps_lib_opt, extra_libs, config->ldlibs, config->ldflags, exe_path, error_file);
#endif
#endif
    }

    if (verbose)
    {
        DEBUG_INFO("Executing: %s", command);
    }

    /* Execute C compiler */
    int result = system(command);

    if (result != 0)
    {
        /* Show compiler error output */
        FILE *errfile = fopen(error_file, "r");
        if (errfile)
        {
            char line[1024];
            fprintf(stderr, "\n");
            while (fgets(line, sizeof(line), errfile))
            {
                fprintf(stderr, "%s", line);
            }
            fclose(errfile);
        }
        unlink(error_file);
        return false;
    }

    /* Clean up error file on success */
    unlink(error_file);

    if (verbose)
    {
        DEBUG_INFO("Successfully compiled to: %s", exe_path);
    }

    return true;
}
