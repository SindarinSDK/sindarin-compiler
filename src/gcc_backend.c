#include "gcc_backend.h"
#include "code_gen.h"
#include "debug.h"
#include "package.h"
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
#include <dirent.h>
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

/* Static buffer for resolved SDK root path */
static char sdk_root_buf[PATH_MAX];

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

/* Get platform name for package library paths */
static const char *get_platform_name(void)
{
#ifdef _WIN32
    return "windows";
#elif defined(__APPLE__)
    return "darwin";
#else
    return "linux";
#endif
}

/* Helper to append an include path to the options buffer */
static void append_include_path(char *pkg_include_opts, size_t inc_size, const char *path)
{
    size_t len = strlen(pkg_include_opts);
    if (len > 0) {
        snprintf(pkg_include_opts + len, inc_size - len, " -I\"%s\"", path);
    } else {
        snprintf(pkg_include_opts, inc_size, "-I\"%s\"", path);
    }
}

/* Helper to append a define to the options buffer */
static void append_define(char *pkg_include_opts, size_t inc_size, const char *define)
{
    size_t len = strlen(pkg_include_opts);
    if (len > 0) {
        snprintf(pkg_include_opts + len, inc_size - len, " %s", define);
    } else {
        snprintf(pkg_include_opts, inc_size, "%s", define);
    }
}

/*
 * Parse pkg-config (.pc) files to extract include paths and defines.
 *
 * The .pc file format uses variable definitions and substitutions:
 *   prefix=/some/path
 *   includedir=${prefix}/include
 *   Cflags: -I${includedir}/libxml2 -DLIBXML_STATIC
 *
 * This function parses all .pc files in a pkgconfig directory and
 * extracts -I (include) and -D (define) flags from the Cflags line.
 */
#define MAX_PC_VARS 32
#define MAX_PC_VAR_LEN 256

/* Normalize a path: convert all separators to platform native and resolve .. */
static void normalize_pc_path(char *path)
{
#ifdef _WIN32
    /* Convert forward slashes to backslashes */
    for (char *p = path; *p; p++) {
        if (*p == '/') *p = '\\';
    }
#endif

    /* Resolve .. components */
    char *out = path;
    char *in = path;
    char *components[64];
    int comp_count = 0;

    /* Parse path into components */
    while (*in) {
        /* Skip separators */
        while (*in == SN_PATH_SEP || *in == '/' || *in == '\\') in++;
        if (!*in) break;

        /* Find end of component */
        char *start = in;
        while (*in && *in != SN_PATH_SEP && *in != '/' && *in != '\\') in++;

        size_t len = in - start;
        if (len == 2 && start[0] == '.' && start[1] == '.') {
            /* Go up one level */
            if (comp_count > 0) comp_count--;
        } else if (len == 1 && start[0] == '.') {
            /* Current directory - skip */
        } else {
            /* Normal component */
            if (comp_count < 64) {
                components[comp_count++] = start;
            }
        }
    }

    /* Rebuild path */
    out = path;
    for (int i = 0; i < comp_count; i++) {
        if (i > 0) *out++ = SN_PATH_SEP;
        char *comp = components[i];
        while (*comp && *comp != SN_PATH_SEP && *comp != '/' && *comp != '\\') {
            *out++ = *comp++;
        }
    }
    *out = '\0';
}

typedef struct {
    char name[64];
    char value[MAX_PC_VAR_LEN];
} PkgConfigVar;

/* Substitute ${varname} in a string using the provided variables */
static void pc_substitute_vars(char *out, size_t out_size, const char *in,
                               PkgConfigVar *vars, int var_count)
{
    size_t out_pos = 0;
    const char *p = in;

    while (*p && out_pos < out_size - 1) {
        if (p[0] == '$' && p[1] == '{') {
            /* Find closing brace */
            const char *end = strchr(p + 2, '}');
            if (end) {
                /* Extract variable name */
                size_t name_len = end - (p + 2);
                char var_name[64];
                if (name_len < sizeof(var_name)) {
                    strncpy(var_name, p + 2, name_len);
                    var_name[name_len] = '\0';

                    /* Look up variable */
                    const char *value = NULL;
                    for (int i = 0; i < var_count; i++) {
                        if (strcmp(vars[i].name, var_name) == 0) {
                            value = vars[i].value;
                            break;
                        }
                    }

                    if (value) {
                        /* Copy substituted value */
                        size_t val_len = strlen(value);
                        if (out_pos + val_len < out_size - 1) {
                            strcpy(out + out_pos, value);
                            out_pos += val_len;
                        }
                        p = end + 1;
                        continue;
                    }
                }
            }
        }
        out[out_pos++] = *p++;
    }
    out[out_pos] = '\0';
}

/* Parse a single .pc file and extract Cflags */
static void parse_pc_file(const char *pc_path, const char *base_dir,
                          char *pkg_include_opts, size_t inc_size)
{
    FILE *f = fopen(pc_path, "r");
    if (!f) return;

    PkgConfigVar vars[MAX_PC_VARS];
    int var_count = 0;

    /* Pre-populate pcfiledir variable (directory containing .pc file) */
    strncpy(vars[var_count].name, "pcfiledir", sizeof(vars[var_count].name));
    strncpy(vars[var_count].value, base_dir, sizeof(vars[var_count].value));
    vars[var_count].value[sizeof(vars[var_count].value) - 1] = '\0';
    var_count++;

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        /* Remove trailing newline/whitespace */
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r' || line[len-1] == ' '))
            line[--len] = '\0';

        /* Skip empty lines and comments */
        if (line[0] == '\0' || line[0] == '#') continue;

        /* Check for variable definition: name=value */
        char *eq = strchr(line, '=');
        if (eq && !strchr(line, ':')) {
            /* This is a variable definition */
            *eq = '\0';
            const char *name = line;
            const char *value = eq + 1;

            /* Substitute any variables in the value */
            char expanded[MAX_PC_VAR_LEN];
            pc_substitute_vars(expanded, sizeof(expanded), value, vars, var_count);

            if (var_count < MAX_PC_VARS) {
                strncpy(vars[var_count].name, name, sizeof(vars[var_count].name) - 1);
                vars[var_count].name[sizeof(vars[var_count].name) - 1] = '\0';
                strncpy(vars[var_count].value, expanded, sizeof(vars[var_count].value) - 1);
                vars[var_count].value[sizeof(vars[var_count].value) - 1] = '\0';
                var_count++;
            }
            continue;
        }

        /* Check for Cflags: line */
        if (strncmp(line, "Cflags:", 7) == 0) {
            const char *cflags = line + 7;
            while (*cflags == ' ' || *cflags == '\t') cflags++;

            /* Substitute variables */
            char expanded[1024];
            pc_substitute_vars(expanded, sizeof(expanded), cflags, vars, var_count);

            /* Parse individual flags - handle quoted flags like "-I/path" */
            char *p = expanded;
            while (*p) {
                /* Skip whitespace */
                while (*p == ' ' || *p == '\t') p++;
                if (!*p) break;

                /* Extract token - handle quoted tokens */
                char token[PATH_MAX];
                size_t tok_len = 0;
                if (*p == '"') {
                    /* Quoted token - find closing quote */
                    p++; /* skip opening quote */
                    while (*p && *p != '"' && tok_len < sizeof(token) - 1) {
                        token[tok_len++] = *p++;
                    }
                    if (*p == '"') p++; /* skip closing quote */
                } else {
                    /* Unquoted token */
                    while (*p && *p != ' ' && *p != '\t' && tok_len < sizeof(token) - 1) {
                        token[tok_len++] = *p++;
                    }
                }
                token[tok_len] = '\0';

                if (strncmp(token, "-I", 2) == 0) {
                    /* Include path */
                    char clean_path[PATH_MAX];
                    strncpy(clean_path, token + 2, sizeof(clean_path) - 1);
                    clean_path[sizeof(clean_path) - 1] = '\0';
                    /* Normalize path separators and resolve .. */
                    normalize_pc_path(clean_path);
                    if (clean_path[0] && dir_exists(clean_path)) {
                        append_include_path(pkg_include_opts, inc_size, clean_path);
                    }
                } else if (strncmp(token, "-D", 2) == 0) {
                    /* Define - pass through as-is */
                    append_define(pkg_include_opts, inc_size, token);
                }
            }
        }
    }

    fclose(f);
}

/* Scan pkgconfig directory and parse all .pc files */
static void parse_pkgconfig_dir(const char *pkgconfig_dir,
                                char *pkg_include_opts, size_t inc_size)
{
    if (!dir_exists(pkgconfig_dir)) return;

#ifdef _WIN32
    char search_path[PATH_MAX];
    snprintf(search_path, sizeof(search_path), "%s\\*.pc", pkgconfig_dir);

    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            char pc_path[PATH_MAX];
            snprintf(pc_path, sizeof(pc_path), "%s\\%s", pkgconfig_dir, find_data.cFileName);
            parse_pc_file(pc_path, pkgconfig_dir, pkg_include_opts, inc_size);
        }
    } while (FindNextFileA(hFind, &find_data));

    FindClose(hFind);
#else
    DIR *dir = opendir(pkgconfig_dir);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        size_t name_len = strlen(entry->d_name);
        if (name_len > 3 && strcmp(entry->d_name + name_len - 3, ".pc") == 0) {
            char pc_path[PATH_MAX];
            snprintf(pc_path, sizeof(pc_path), "%s/%s", pkgconfig_dir, entry->d_name);
            parse_pc_file(pc_path, pkgconfig_dir, pkg_include_opts, inc_size);
        }
    }

    closedir(dir);
#endif
}

/* Build package library include/lib paths from sn.yaml dependencies.
 * Appends -I and -L/-rpath flags to the provided buffers.
 * Returns true if any package paths were added. */
static bool build_package_lib_paths(char *pkg_include_opts, size_t inc_size,
                                    char *pkg_lib_opts, size_t lib_size)
{
    pkg_include_opts[0] = '\0';
    pkg_lib_opts[0] = '\0';

    /* Check if sn.yaml exists in current directory */
    if (!package_yaml_exists()) {
        return false;
    }

    /* Parse package config */
    PackageConfig config;
    if (!package_yaml_parse("sn.yaml", &config)) {
        return false;
    }

    if (config.dependency_count == 0) {
        return false;
    }

    const char *platform = get_platform_name();
    bool added = false;

    for (int i = 0; i < config.dependency_count; i++) {
        char pkg_include_dir[PATH_MAX];
        char pkg_lib_dir[PATH_MAX];
        char pkg_pkgconfig_dir[PATH_MAX];

        /* Build paths: .sn/<pkg-name>/libs/<platform>/include and lib */
        snprintf(pkg_include_dir, sizeof(pkg_include_dir),
                 ".sn" SN_PATH_SEP_STR "%s" SN_PATH_SEP_STR "libs" SN_PATH_SEP_STR "%s" SN_PATH_SEP_STR "include",
                 config.dependencies[i].name, platform);
        snprintf(pkg_lib_dir, sizeof(pkg_lib_dir),
                 ".sn" SN_PATH_SEP_STR "%s" SN_PATH_SEP_STR "libs" SN_PATH_SEP_STR "%s" SN_PATH_SEP_STR "lib",
                 config.dependencies[i].name, platform);
        snprintf(pkg_pkgconfig_dir, sizeof(pkg_pkgconfig_dir),
                 ".sn" SN_PATH_SEP_STR "%s" SN_PATH_SEP_STR "libs" SN_PATH_SEP_STR "%s" SN_PATH_SEP_STR "lib" SN_PATH_SEP_STR "pkgconfig",
                 config.dependencies[i].name, platform);

        /* Add base include directory if it exists */
        if (dir_exists(pkg_include_dir)) {
            append_include_path(pkg_include_opts, inc_size, pkg_include_dir);
            added = true;
        }

        /* Parse pkg-config files to get additional include paths and defines */
        if (dir_exists(pkg_pkgconfig_dir)) {
            parse_pkgconfig_dir(pkg_pkgconfig_dir, pkg_include_opts, inc_size);
        }

        if (dir_exists(pkg_lib_dir)) {
            size_t len = strlen(pkg_lib_opts);
#ifdef __APPLE__
            if (len > 0) {
                snprintf(pkg_lib_opts + len, lib_size - len, " -L\"%s\" -Wl,-rpath,\"%s\"", pkg_lib_dir, pkg_lib_dir);
            } else {
                snprintf(pkg_lib_opts, lib_size, "-L\"%s\" -Wl,-rpath,\"%s\"", pkg_lib_dir, pkg_lib_dir);
            }
#else
            if (len > 0) {
                snprintf(pkg_lib_opts + len, lib_size - len, " -L\"%s\" -Wl,-rpath,\"%s\"", pkg_lib_dir, pkg_lib_dir);
            } else {
                snprintf(pkg_lib_opts, lib_size, "-L\"%s\" -Wl,-rpath,\"%s\"", pkg_lib_dir, pkg_lib_dir);
            }
#endif
            added = true;
        }
    }

    return added;
}

/*
 * Platform-specific library name translation.
 *
 * Some libraries have different names on different platforms:
 * - zlib: "z" on Unix, "zlib" on Windows (upstream zlib convention)
 *
 * This function translates SDK library names to platform-appropriate names.
 * Returns the translated name, or the original if no translation is needed.
 */
static const char *translate_lib_name(const char *lib)
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

/* Get the SDK root directory.
 * This is the single location where all Sindarin resources live:
 *   $SN_SDK/sn.cfg           - Configuration file
 *   $SN_SDK/lib/<backend>/   - Runtime libraries
 *   $SN_SDK/include/         - Runtime headers
 *   $SN_SDK/sdk/             - SDK standard library modules
 *   $SN_SDK/deps/            - Bundled dependencies (zlib, yyjson)
 *
 * Resolution order:
 *   1. $SN_SDK environment variable (if set and directory exists)
 *   2. Compiler executable's directory (portable/development mode)
 *
 * Returns path to SDK root directory.
 * The returned path is statically allocated - do not free.
 */
static const char *get_sdk_root(const char *compiler_dir)
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



/* Cached SDK root path (computed once per session) */
static const char *cached_sdk_root = NULL;

/* Resolve an SDK import to its full file path.
 * Given a module name (e.g., "math"), returns the full path to the SDK file
 * (e.g., "$SN_SDK/sdk/math.sn") if it exists.
 *
 * Parameters:
 *   compiler_dir - Directory containing the compiler executable (fallback for SN_SDK)
 *   module_name  - Name of the module to import (without .sn extension)
 *
 * Returns path to SDK file or NULL if not found.
 * The returned path is statically allocated - do not free.
 */
const char *gcc_resolve_sdk_import(const char *compiler_dir, const char *module_name)
{
    /* Cache the SDK root on first call */
    if (!cached_sdk_root)
    {
        cached_sdk_root = get_sdk_root(compiler_dir);
    }

    /* Strip "sdk/" prefix if present - we add it ourselves */
    const char *stripped_name = module_name;
    if (strncmp(module_name, "sdk/", 4) == 0)
    {
        stripped_name = module_name + 4;
    }
    else if (strncmp(module_name, "sdk\\", 4) == 0)
    {
        stripped_name = module_name + 4;
    }

    /* Construct the full path: $SN_SDK/sdk/<module>.sn */
    snprintf(sdk_file_buf, sizeof(sdk_file_buf), "%s" SN_PATH_SEP_STR "sdk" SN_PATH_SEP_STR "%s.sn",
             cached_sdk_root, stripped_name);

    if (file_exists(sdk_file_buf))
    {
        return sdk_file_buf;
    }

    return NULL; /* SDK module not found */
}

/* Reset the SDK root cache (for testing purposes) */
void gcc_reset_sdk_cache(void)
{
    cached_sdk_root = NULL;
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

/* Load config file from SDK root directory if it exists */
void cc_backend_load_config(const char *compiler_dir)
{
    if (cfg_loaded) return;
    cfg_loaded = true;

    /* Config file lives at $SN_SDK/sn.cfg */
    const char *sdk_root = get_sdk_root(compiler_dir);
    char config_path[PATH_MAX];
    snprintf(config_path, sizeof(config_path), "%s" SN_PATH_SEP_STR "sn.cfg", sdk_root);

    /* Try to open config file */
    FILE *f = fopen(config_path, "r");
    if (!f) return; /* No config file found, use defaults */

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

    /* All paths are relative to the SDK root ($SN_SDK or compiler directory) */
    const char *sdk_root = get_sdk_root(compiler_dir);

    /* Build runtime library directory: $SN_SDK/lib/<backend>/ */
    snprintf(lib_dir, sizeof(lib_dir), "%s" SN_PATH_SEP_STR "%s", sdk_root, backend_lib_subdir(backend));

    /* Build include directory: $SN_SDK/include/ */
    snprintf(include_dir, sizeof(include_dir), "%s" SN_PATH_SEP_STR "include", sdk_root);

    /* Build deps directories: $SN_SDK/deps/include/ and $SN_SDK/deps/lib/ */
    deps_include_dir[0] = '\0';
    deps_lib_dir[0] = '\0';
    snprintf(deps_include_dir, sizeof(deps_include_dir), "%s" SN_PATH_SEP_STR "deps" SN_PATH_SEP_STR "include", sdk_root);
    if (dir_exists(deps_include_dir))
    {
        snprintf(deps_lib_dir, sizeof(deps_lib_dir), "%s" SN_PATH_SEP_STR "deps" SN_PATH_SEP_STR "lib", sdk_root);
    }
    else
    {
        deps_include_dir[0] = '\0';
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
            /* Translate library name for current platform (e.g., "z" -> "zlib" on Windows) */
            const char *lib = translate_lib_name(link_libs[i]);
            int written = snprintf(extra_libs + offset, sizeof(extra_libs) - offset, " -l%s", lib);
            if (written > 0)
            {
                offset += written;
            }
        }
    }

    /* Append transitive dependencies for statically-linked libraries.
     * When linking OpenSSL (ssl/crypto), we need platform-specific deps. */
    if (link_libs != NULL && link_lib_count > 0)
    {
        bool needs_openssl_deps = false;
        for (int i = 0; i < link_lib_count; i++)
        {
            if (strcmp(link_libs[i], "ssl") == 0 || strcmp(link_libs[i], "crypto") == 0 ||
                strcmp(link_libs[i], "ngtcp2") == 0 || strcmp(link_libs[i], "ngtcp2_crypto_ossl") == 0)
            {
                needs_openssl_deps = true;
                break;
            }
        }
        if (needs_openssl_deps)
        {
            int offset = (int)strlen(extra_libs);
#ifdef _WIN32
            int written = snprintf(extra_libs + offset, sizeof(extra_libs) - offset, " -lcrypt32");
#elif defined(__APPLE__)
            int written = snprintf(extra_libs + offset, sizeof(extra_libs) - offset,
                " -framework Security -framework CoreFoundation");
#else
            int written = snprintf(extra_libs + offset, sizeof(extra_libs) - offset, " -ldl");
#endif
            (void)written;
        }
    }

    /* When linking libssh, add platform-specific transitive deps */
    if (link_libs != NULL && link_lib_count > 0)
    {
        bool needs_ssh_deps = false;
        for (int i = 0; i < link_lib_count; i++)
        {
            if (strcmp(link_libs[i], "ssh") == 0)
            {
                needs_ssh_deps = true;
                break;
            }
        }
        if (needs_ssh_deps)
        {
            int offset = (int)strlen(extra_libs);
#ifdef _WIN32
            int written = snprintf(extra_libs + offset, sizeof(extra_libs) - offset,
                " -lzlib -lbcrypt -lws2_32 -liphlpapi");
#else
            int written = snprintf(extra_libs + offset, sizeof(extra_libs) - offset,
                " -lz -lpthread");
#endif
            (void)written;
        }
    }

    /* When linking libgit2, add platform-specific transitive deps */
    if (link_libs != NULL && link_lib_count > 0)
    {
        bool needs_git2_deps = false;
        for (int i = 0; i < link_lib_count; i++)
        {
            if (strcmp(link_libs[i], "git2") == 0)
            {
                needs_git2_deps = true;
                break;
            }
        }
        if (needs_git2_deps)
        {
            int offset = (int)strlen(extra_libs);
#ifdef _WIN32
            int written = snprintf(extra_libs + offset, sizeof(extra_libs) - offset,
                " -lhttp_parser -lssh2 -lpcre2-8 -lzlib -lssl -lcrypto"
                " -lws2_32 -lsecur32 -lbcrypt -lcrypt32 -lrpcrt4 -lole32");
#elif defined(__APPLE__)
            int written = snprintf(extra_libs + offset, sizeof(extra_libs) - offset,
                " -lhttp_parser -lssh2 -lpcre2-8 -lz -lssl -lcrypto -liconv"
                " -framework Security -framework CoreFoundation");
#else
            int written = snprintf(extra_libs + offset, sizeof(extra_libs) - offset,
                " -lhttp_parser -lssh2 -lpcre2-8 -lz -lssl -lcrypto -lpthread -ldl");
#endif
            (void)written;
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
    char deps_lib_opt[PATH_MAX * 2 + 32];
    if (deps_include_dir[0])
        snprintf(deps_include_opt, sizeof(deps_include_opt), "-I\"%s\"", deps_include_dir);
    else
        deps_include_opt[0] = '\0';
    if (deps_lib_dir[0])
    {
        /* Add both -L (link-time) and -rpath (run-time) so executables can find shared libs */
#ifdef __APPLE__
        snprintf(deps_lib_opt, sizeof(deps_lib_opt), "-L\"%s\" -Wl,-rpath,\"%s\"", deps_lib_dir, deps_lib_dir);
#else
        snprintf(deps_lib_opt, sizeof(deps_lib_opt), "-L\"%s\" -Wl,-rpath,\"%s\"", deps_lib_dir, deps_lib_dir);
#endif
    }
    else
        deps_lib_opt[0] = '\0';

    /* Build package library paths from sn.yaml dependencies */
    char pkg_include_opt[PATH_MAX * 4];
    char pkg_lib_opt[PATH_MAX * 4];
    build_package_lib_paths(pkg_include_opt, sizeof(pkg_include_opt),
                            pkg_lib_opt, sizeof(pkg_lib_opt));

    if (verbose && pkg_include_opt[0])
    {
        DEBUG_INFO("Package includes: %s", pkg_include_opt);
        DEBUG_INFO("Package libs: %s", pkg_lib_opt);
    }

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
        /* TODO: Add MSVC-style package include options (convert -I to /I) */
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
            "%s%s%s %s -w -std=%s -DSN_USE_WIN32_THREADS %s -I\"%s\" %s %s "
            "\"%s\"%s -Wl,--whole-archive \"%s\" -Wl,--no-whole-archive "
            "%s %s -lpthread -lm%s %s %s -o \"%s\" 2>\"%s\"",
            cc_quote, config->cc, cc_quote, mode_cflags, config->std, config->cflags, include_dir, deps_include_opt, pkg_include_opt,
            c_file_normalized, extra_sources, runtime_lib,
            deps_lib_opt, pkg_lib_opt, extra_libs, config->ldlibs, config->ldflags, exe_path, error_file);
#else
        /* Unix: Use -Wl,--whole-archive (Linux) or just link normally (macOS handles it) */
#ifdef __APPLE__
        snprintf(command, sizeof(command),
            "%s%s%s %s -w -std=%s -D_GNU_SOURCE %s -I\"%s\" %s %s "
            "\"%s\"%s -Wl,-force_load,\"%s\" "
            "%s %s -lpthread -lm%s %s %s -o \"%s\" 2>\"%s\"",
            cc_quote, config->cc, cc_quote, mode_cflags, config->std, config->cflags, include_dir, deps_include_opt, pkg_include_opt,
            c_file_normalized, extra_sources, runtime_lib,
            deps_lib_opt, pkg_lib_opt, extra_libs, config->ldlibs, config->ldflags, exe_path, error_file);
#else
        snprintf(command, sizeof(command),
            "%s%s%s %s -w -std=%s -D_GNU_SOURCE %s -I\"%s\" %s %s "
            "\"%s\"%s -Wl,--whole-archive \"%s\" -Wl,--no-whole-archive "
            "%s %s -lpthread -lm%s %s %s -o \"%s\" 2>\"%s\"",
            cc_quote, config->cc, cc_quote, mode_cflags, config->std, config->cflags, include_dir, deps_include_opt, pkg_include_opt,
            c_file_normalized, extra_sources, runtime_lib,
            deps_lib_opt, pkg_lib_opt, extra_libs, config->ldlibs, config->ldflags, exe_path, error_file);
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
