#include "gcc_backend.h"
#include "gcc_backend_config.h"
#include "gcc_backend_pkgconfig.h"
#include "debug.h"
#include "package.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "platform/platform.h"
    #if defined(__MINGW32__) || defined(__MINGW64__)
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

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

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

#ifdef _WIN32
#define SN_PATH_SEP '\\'
#define SN_PATH_SEP_STR "\\"
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

/* Static buffer for resolved SDK file path */
static char sdk_file_buf[PATH_MAX];

/* Helper: check if file exists and is readable */
static bool file_exists(const char *path)
{
    return access(path, R_OK) == 0;
}

/* Default values for backend configuration (fallback when no platform config is found) */
#define DEFAULT_STD "c99"
#ifdef __APPLE__
#define DEFAULT_DEBUG_CFLAGS_GCC "-fwrapv -fno-omit-frame-pointer -g"
#define DEFAULT_DEBUG_CFLAGS_CLANG "-fwrapv -fno-omit-frame-pointer -g"
#define DEFAULT_LDLIBS_UNIX "-lpthread"
#else
#define DEFAULT_DEBUG_CFLAGS_GCC "-fwrapv -no-pie -fsanitize=address -fno-omit-frame-pointer -g"
#define DEFAULT_DEBUG_CFLAGS_CLANG "-fwrapv -fsanitize=address -fno-omit-frame-pointer -g"
#define DEFAULT_LDLIBS_UNIX "-lpthread -lm"
#endif
#define DEFAULT_RELEASE_CFLAGS_GCC "-O3 -flto -fwrapv"
#define DEFAULT_RELEASE_CFLAGS_CLANG "-O3 -flto -fwrapv"
#define DEFAULT_PROFILE_CFLAGS_GCC "-O2 -fno-omit-frame-pointer -g -fwrapv"
#define DEFAULT_PROFILE_CFLAGS_CLANG "-O2 -fno-omit-frame-pointer -g -fwrapv"
#define DEFAULT_DEBUG_CFLAGS_TCC "-g"
#define DEFAULT_RELEASE_CFLAGS_TCC "-O2"
#define DEFAULT_PROFILE_CFLAGS_TCC "-O2 -g"
#define DEFAULT_DEBUG_CFLAGS_MSVC "/Zi /Od"
#define DEFAULT_RELEASE_CFLAGS_MSVC "/O2 /DNDEBUG"
#define DEFAULT_PROFILE_CFLAGS_MSVC "/O2 /Zi"
#define DEFAULT_CFLAGS_MSVC "/W3 /D_CRT_SECURE_NO_WARNINGS"
#define DEFAULT_LDLIBS_WIN "ws2_32.lib bcrypt.lib"
#define DEFAULT_LDLIBS_CLANG_WIN "-lws2_32 -lbcrypt -lpthread"
#define DEFAULT_LDLIBS_GCC_WIN "-lws2_32 -lbcrypt -lpthread"

/* Static buffers for config file values */
static char cfg_cc[256];
static char cfg_std[64];
static char cfg_debug_cflags[1024];
static char cfg_release_cflags[1024];
static char cfg_profile_cflags[1024];
static char cfg_cflags[1024];
static char cfg_ldflags[1024];
static char cfg_ldlibs[1024];
static bool cfg_loaded = false;

/* SN_LDLIBS_<name> table: full link flags for each @link library */
#define MAX_LDLIBS_ENTRIES 64
typedef struct { char lib[64]; char flags[1024]; } LdlibsEntry;
static LdlibsEntry cfg_ldlibs_table[MAX_LDLIBS_ENTRIES];
static int cfg_ldlibs_count = 0;

/* Return config-driven link flags for a library, or NULL if not in table */
static const char *get_ldlibs_for_lib(const char *lib)
{
    for (int i = 0; i < cfg_ldlibs_count; i++)
    {
        if (strcmp(cfg_ldlibs_table[i].lib, lib) == 0)
            return cfg_ldlibs_table[i].flags;
    }
    return NULL;
}

/* Cached SDK root path (computed once per session) */
static const char *cached_sdk_root = NULL;

const char *gcc_resolve_sdk_import(const char *compiler_dir, const char *module_name)
{
    if (!cached_sdk_root)
    {
        cached_sdk_root = get_sdk_root(compiler_dir);
    }

    const char *stripped_name = module_name;
    if (strncmp(module_name, "sdk/", 4) == 0)
    {
        stripped_name = module_name + 4;
    }
    else if (strncmp(module_name, "sdk\\", 4) == 0)
    {
        stripped_name = module_name + 4;
    }

    snprintf(sdk_file_buf, sizeof(sdk_file_buf), "%s" SN_PATH_SEP_STR "sdk" SN_PATH_SEP_STR "%s.sn",
             cached_sdk_root, stripped_name);

    if (file_exists(sdk_file_buf))
    {
        return sdk_file_buf;
    }

    return NULL;
}

void gcc_reset_sdk_cache(void)
{
    cached_sdk_root = NULL;
}

static void parse_config_line(const char *line)
{
    while (*line == ' ' || *line == '\t') line++;

    if (*line == '\0' || *line == '\n' || *line == '#')
        return;

    const char *eq = strchr(line, '=');
    if (!eq) return;

    size_t key_len = eq - line;
    const char *value = eq + 1;

    size_t value_len = strlen(value);
    while (value_len > 0 && (value[value_len-1] == '\n' || value[value_len-1] == '\r' ||
                              value[value_len-1] == ' ' || value[value_len-1] == '\t'))
    {
        value_len--;
    }

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
    else if (key_len == 17 && strncmp(line, "SN_PROFILE_CFLAGS", 17) == 0)
    {
        if (value_len < sizeof(cfg_profile_cflags))
        {
            strncpy(cfg_profile_cflags, value, value_len);
            cfg_profile_cflags[value_len] = '\0';
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
    else if (key_len > 10 && strncmp(line, "SN_LDLIBS_", 10) == 0)
    {
        if (cfg_ldlibs_count < MAX_LDLIBS_ENTRIES)
        {
            size_t lib_len = key_len - 10;
            if (lib_len < sizeof(cfg_ldlibs_table[0].lib))
            {
                LdlibsEntry *e = &cfg_ldlibs_table[cfg_ldlibs_count];
                strncpy(e->lib, line + 10, lib_len);
                e->lib[lib_len] = '\0';
                if (value_len < sizeof(e->flags))
                {
                    strncpy(e->flags, value, value_len);
                    e->flags[value_len] = '\0';
                }
                cfg_ldlibs_count++;
            }
        }
    }
}

static void load_config_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[2048];
    while (fgets(line, sizeof(line), f))
        parse_config_line(line);
    fclose(f);
}

void cc_backend_load_config(const char *compiler_dir)
{
    if (cfg_loaded) return;
    cfg_loaded = true;

#ifdef _WIN32
    const char *platform = "windows";
#elif defined(__APPLE__)
    const char *platform = "darwin";
#else
    const char *platform = "linux";
#endif

    const char *sdk_root = get_sdk_root(compiler_dir);
    char config_path[PATH_MAX];
    snprintf(config_path, sizeof(config_path), "%s" SN_PATH_SEP_STR "sn.%s.cfg", sdk_root, platform);
    load_config_file(config_path);
}

void cc_backend_init_config(CCBackendConfig *config)
{
    const char *env_val;

    const char *actual_cc;
    env_val = getenv("SN_CC");
    if (env_val && env_val[0])
        actual_cc = env_val;
    else if (cfg_cc[0])
        actual_cc = cfg_cc;
    else
        actual_cc = NULL;

    BackendType backend;
    if (actual_cc)
        backend = detect_backend(actual_cc);
    else
        backend = detect_backend_from_exe();

    const char *default_cc;
    const char *default_debug_cflags;
    const char *default_release_cflags;
    const char *default_profile_cflags;
    const char *default_cflags = "";
    const char *default_ldlibs = "";

    switch (backend)
    {
        case BACKEND_CLANG:
            default_cc = "clang";
            default_debug_cflags = DEFAULT_DEBUG_CFLAGS_CLANG;
            default_release_cflags = DEFAULT_RELEASE_CFLAGS_CLANG;
            default_profile_cflags = DEFAULT_PROFILE_CFLAGS_CLANG;
#ifdef _WIN32
            default_ldlibs = DEFAULT_LDLIBS_CLANG_WIN;
#else
            default_ldlibs = DEFAULT_LDLIBS_UNIX;
#endif
            break;
        case BACKEND_TINYCC:
            default_cc = "tcc";
            default_debug_cflags = DEFAULT_DEBUG_CFLAGS_TCC;
            default_release_cflags = DEFAULT_RELEASE_CFLAGS_TCC;
            default_profile_cflags = DEFAULT_PROFILE_CFLAGS_TCC;
            default_ldlibs = DEFAULT_LDLIBS_UNIX;
            break;
        case BACKEND_MSVC:
            default_cc = "cl";
            default_debug_cflags = DEFAULT_DEBUG_CFLAGS_MSVC;
            default_release_cflags = DEFAULT_RELEASE_CFLAGS_MSVC;
            default_profile_cflags = DEFAULT_PROFILE_CFLAGS_MSVC;
            default_cflags = DEFAULT_CFLAGS_MSVC;
            default_ldlibs = DEFAULT_LDLIBS_WIN;
            break;
        case BACKEND_GCC:
        default:
            default_cc = "gcc";
            default_debug_cflags = DEFAULT_DEBUG_CFLAGS_GCC;
            default_release_cflags = DEFAULT_RELEASE_CFLAGS_GCC;
            default_profile_cflags = DEFAULT_PROFILE_CFLAGS_GCC;
#ifdef _WIN32
            default_ldlibs = DEFAULT_LDLIBS_GCC_WIN;
#else
            default_ldlibs = DEFAULT_LDLIBS_UNIX;
#endif
            break;
    }

    if (actual_cc)
        config->cc = actual_cc;
    else
        config->cc = default_cc;

    env_val = getenv("SN_STD");
    if (env_val && env_val[0])
        config->std = env_val;
    else if (cfg_std[0])
        config->std = cfg_std;
    else
        config->std = DEFAULT_STD;

    env_val = getenv("SN_DEBUG_CFLAGS");
    if (env_val && env_val[0])
        config->debug_cflags = env_val;
    else if (cfg_debug_cflags[0])
        config->debug_cflags = cfg_debug_cflags;
    else
        config->debug_cflags = default_debug_cflags;

    env_val = getenv("SN_RELEASE_CFLAGS");
    if (env_val && env_val[0])
        config->release_cflags = env_val;
    else if (cfg_release_cflags[0])
        config->release_cflags = cfg_release_cflags;
    else
        config->release_cflags = default_release_cflags;

    env_val = getenv("SN_PROFILE_CFLAGS");
    if (env_val && env_val[0])
        config->profile_cflags = env_val;
    else if (cfg_profile_cflags[0])
        config->profile_cflags = cfg_profile_cflags;
    else
        config->profile_cflags = default_profile_cflags;

    env_val = getenv("SN_CFLAGS");
    if (env_val && env_val[0])
        config->cflags = env_val;
    else if (cfg_cflags[0])
        config->cflags = cfg_cflags;
    else
        config->cflags = default_cflags;

    env_val = getenv("SN_LDFLAGS");
    if (env_val && env_val[0])
        config->ldflags = env_val;
    else if (cfg_ldflags[0])
        config->ldflags = cfg_ldflags;
    else
        config->ldflags = "";

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
    char check_cmd[PATH_MAX];

    bool is_msvc = (strcmp(config->cc, "cl") == 0 || strstr(config->cc, "cl.exe") != NULL);

#ifdef _WIN32
    const char *cc_quote = (strchr(config->cc, ' ') != NULL) ? "\"" : "";
    if (is_msvc)
    {
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

const char *gcc_get_compiler_dir(const char *argv0)
{
#ifdef _WIN32
    ssize_t len = sn_get_executable_path(compiler_dir_buf, sizeof(compiler_dir_buf));
    if (len != -1)
    {
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
                const char *path = real_path;
                if (strncmp(path, "\\\\?\\", 4) == 0)
                    path += 4;
                strncpy(compiler_dir_buf, path, sizeof(compiler_dir_buf) - 1);
                compiler_dir_buf[sizeof(compiler_dir_buf) - 1] = '\0';
            }
        }
        char *dir = dirname(compiler_dir_buf);
        memmove(compiler_dir_buf, dir, strlen(dir) + 1);
        return compiler_dir_buf;
    }
#elif defined(__APPLE__)
    uint32_t size = sizeof(compiler_dir_buf);
    if (_NSGetExecutablePath(compiler_dir_buf, &size) == 0)
    {
        char real_path[PATH_MAX];
        if (realpath(compiler_dir_buf, real_path) != NULL)
        {
            strncpy(compiler_dir_buf, real_path, sizeof(compiler_dir_buf) - 1);
            compiler_dir_buf[sizeof(compiler_dir_buf) - 1] = '\0';
        }
        char *dir = dirname(compiler_dir_buf);
        memmove(compiler_dir_buf, dir, strlen(dir) + 1);
        return compiler_dir_buf;
    }
#else
    ssize_t len = readlink("/proc/self/exe", compiler_dir_buf, sizeof(compiler_dir_buf) - 1);
    if (len != -1)
    {
        compiler_dir_buf[len] = '\0';
        char *dir = dirname(compiler_dir_buf);
        memmove(compiler_dir_buf, dir, strlen(dir) + 1);
        return compiler_dir_buf;
    }
#endif

    if (argv0 != NULL)
    {
        strncpy(compiler_dir_buf, argv0, sizeof(compiler_dir_buf) - 1);
        compiler_dir_buf[sizeof(compiler_dir_buf) - 1] = '\0';
        char *dir = dirname(compiler_dir_buf);
        memmove(compiler_dir_buf, dir, strlen(dir) + 1);
        return compiler_dir_buf;
    }

    strcpy(compiler_dir_buf, ".");
    return compiler_dir_buf;
}

/* Check if the resolved compiler_dir actually contains templates.
 * On Windows (and any install where the binary is copied rather than
 * symlinked), the exe may live in bin/ while templates are in
 * ../lib/sindarin/. Fall back to that path if needed. */
void gcc_resolve_compiler_dir(char *dir_buf, int buf_size)
{
    char check_path[PATH_MAX];
    snprintf(check_path, sizeof(check_path), "%s/templates/c", dir_buf);

    /* If templates exist at the resolved dir, nothing to do */
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(check_path);
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY))
        return;
#else
    if (access(check_path, F_OK) == 0)
        return;
#endif

    /* Try ../lib/sindarin/ relative to the resolved dir */
    char fallback[PATH_MAX];
    snprintf(fallback, sizeof(fallback), "%s/../lib/sindarin", dir_buf);

    char fallback_check[PATH_MAX];
    snprintf(fallback_check, sizeof(fallback_check), "%s/templates/c", fallback);

#ifdef _WIN32
    attrs = GetFileAttributesA(fallback_check);
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY))
    {
        /* Normalize the path */
        char resolved[PATH_MAX];
        if (GetFullPathNameA(fallback, sizeof(resolved), resolved, NULL))
            strncpy(dir_buf, resolved, buf_size - 1);
        else
            strncpy(dir_buf, fallback, buf_size - 1);
        dir_buf[buf_size - 1] = '\0';
    }
#else
    char resolved[PATH_MAX];
    if (realpath(fallback, resolved) != NULL)
    {
        snprintf(fallback_check, sizeof(fallback_check), "%s/templates/c", resolved);
        if (access(fallback_check, F_OK) == 0)
        {
            strncpy(dir_buf, resolved, buf_size - 1);
            dir_buf[buf_size - 1] = '\0';
        }
    }
#endif
}

/* ---- Compilation ---- */

/* Helper: run a command and check result */
static bool run_compile_cmd(const char *command, const char *error_file, bool verbose)
{
    if (verbose)
        DEBUG_INFO("Executing: %s", command);

    int result = system(command);
    if (result != 0)
    {
        FILE *errfile = fopen(error_file, "r");
        if (errfile)
        {
            char line[1024];
            fprintf(stderr, "\n");
            while (fgets(line, sizeof(line), errfile))
                fprintf(stderr, "%s", line);
            fclose(errfile);
        }
        unlink(error_file);
        return false;
    }
    unlink(error_file);
    return true;
}

bool gcc_compile_modular(const CCBackendConfig *config, const char *build_dir,
                          const char **c_files, int c_file_count,
                          const char *output_exe, const char *compiler_dir,
                          bool verbose, bool debug_mode, bool profile_mode,
                          char **link_libs, int link_lib_count,
                          char **pragma_sources, char **pragma_dirs, int pragma_count)
{
    char lib_dir[PATH_MAX];
    char include_dir[PATH_MAX];
    char deps_include_dir[PATH_MAX];
    char deps_lib_dir[PATH_MAX];
    char runtime_lib[PATH_MAX];
    char command[8192];
    char filtered_mode_cflags[1024];

    BackendType backend = detect_backend(config->cc);
    const char *sdk_root = get_sdk_root(compiler_dir);

    snprintf(lib_dir, sizeof(lib_dir), "%s" SN_PATH_SEP_STR "%s", sdk_root, backend_lib_subdir(backend));
    snprintf(include_dir, sizeof(include_dir),
        "%s" SN_PATH_SEP_STR "include" SN_PATH_SEP_STR "minimal\" "
        "-I\"%s" SN_PATH_SEP_STR "include" SN_PATH_SEP_STR "platform\" "
        "-I\"%s" SN_PATH_SEP_STR "include",
        sdk_root, sdk_root, sdk_root);

    deps_include_dir[0] = '\0';
    deps_lib_dir[0] = '\0';
    snprintf(deps_include_dir, sizeof(deps_include_dir), "%s" SN_PATH_SEP_STR "deps" SN_PATH_SEP_STR "include", sdk_root);
    if (access(deps_include_dir, R_OK) == 0)
    {
        snprintf(deps_lib_dir, sizeof(deps_lib_dir), "%s" SN_PATH_SEP_STR "deps" SN_PATH_SEP_STR "lib", sdk_root);
    }
    else
    {
        deps_include_dir[0] = '\0';
    }

    normalize_path_separators(lib_dir);
    normalize_path_separators(include_dir);
    if (deps_include_dir[0]) normalize_path_separators(deps_include_dir);
    if (deps_lib_dir[0]) normalize_path_separators(deps_lib_dir);

    snprintf(runtime_lib, sizeof(runtime_lib), "%s" SN_PATH_SEP_STR "libsn_runtime_min.a", lib_dir);

    if (backend != BACKEND_MSVC && access(runtime_lib, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime library not found: %s\n", runtime_lib);
        fprintf(stderr, "The '%s' backend runtime is not built.\n", backend_name(backend));
        fprintf(stderr, "Run 'make build' to build the runtime.\n");
        return false;
    }

    const char *mode_cflags = profile_mode ? config->profile_cflags
                            : debug_mode  ? config->debug_cflags
                            :               config->release_cflags;
    if (backend == BACKEND_TINYCC)
    {
        mode_cflags = filter_tinycc_flags(mode_cflags, filtered_mode_cflags, sizeof(filtered_mode_cflags));
    }

    char deps_include_opt[PATH_MAX + 8];
    char deps_lib_opt[PATH_MAX * 2 + 32];
    if (deps_include_dir[0])
        snprintf(deps_include_opt, sizeof(deps_include_opt), "-I\"%s\"", deps_include_dir);
    else
        deps_include_opt[0] = '\0';
    if (deps_lib_dir[0])
        snprintf(deps_lib_opt, sizeof(deps_lib_opt), "-L\"%s\" -Wl,-rpath,\"%s\"", deps_lib_dir, deps_lib_dir);
    else
        deps_lib_opt[0] = '\0';

    char pkg_include_opt[16384];
    char pkg_lib_opt[16384];
    build_package_lib_paths(pkg_include_opt, sizeof(pkg_include_opt),
                            pkg_lib_opt, sizeof(pkg_lib_opt));

    if (verbose && pkg_include_opt[0])
    {
        DEBUG_INFO("Package includes: %s", pkg_include_opt);
        DEBUG_INFO("Package libs: %s", pkg_lib_opt);
    }

    char error_file[PATH_MAX];
#ifdef _WIN32
    const char *temp_dir = getenv("TEMP");
    if (!temp_dir) temp_dir = getenv("TMP");
    if (!temp_dir) temp_dir = ".";
    snprintf(error_file, sizeof(error_file), "%s\\sn_cc_errors_%d.txt", temp_dir, (int)getpid());
#else
    strcpy(error_file, "/tmp/sn_cc_errors_XXXXXX");
    int error_fd = mkstemp(error_file);
    if (error_fd == -1)
        strcpy(error_file, "/tmp/sn_cc_errors.txt");
    else
        close(error_fd);
#endif

    const char *cc_quote = (strchr(config->cc, ' ') != NULL) ? "\"" : "";

    /* ---- Step 1: Compile each .c file to .o ---- */
    char *obj_files[512];
    int obj_count = 0;

    for (int i = 0; i < c_file_count && obj_count < 510; i++)
    {
        char c_path[PATH_MAX];
        char o_path[PATH_MAX];
        snprintf(c_path, sizeof(c_path), "%s" SN_PATH_SEP_STR "%s", build_dir, c_files[i]);
        snprintf(o_path, sizeof(o_path), "%s" SN_PATH_SEP_STR "%.*s.o", build_dir,
                 (int)(strlen(c_files[i]) - 2), c_files[i]);

        snprintf(command, sizeof(command),
            "%s%s%s -c %s -w -Werror=implicit-function-declaration -std=%s -D_GNU_SOURCE %s "
            "-I\"%s\" -I\"%s\" %s %s "
            "\"%s\" -o \"%s\" 2>\"%s\"",
            cc_quote, config->cc, cc_quote, mode_cflags, config->std, config->cflags,
            build_dir, include_dir, deps_include_opt, pkg_include_opt,
            c_path, o_path, error_file);

        if (!run_compile_cmd(command, error_file, verbose))
        {
            fprintf(stderr, "Error: failed to compile %s\n", c_files[i]);
            for (int j = 0; j < obj_count; j++) free(obj_files[j]);
            return false;
        }

        obj_files[obj_count] = strdup(o_path);
        obj_count++;
    }

    /* ---- Step 2: Compile pragma source files ---- */
    for (int i = 0; i < pragma_count && obj_count < 510; i++)
    {
        const char *src = pragma_sources[i];
        const char *source_dir = pragma_dirs[i];

        /* Unquote if needed */
        size_t len = strlen(src);
        char unquoted[PATH_MAX];
        if (len >= 2 && src[0] == '"' && src[len - 1] == '"')
        {
            strncpy(unquoted, src + 1, len - 2);
            unquoted[len - 2] = '\0';
            src = unquoted;
        }

        /* Resolve full path */
        char full_path[PATH_MAX];
#ifdef _WIN32
        if (src[0] == '/' || (src[0] && src[1] == ':'))
#else
        if (src[0] == '/')
#endif
        {
            strncpy(full_path, src, sizeof(full_path) - 1);
            full_path[sizeof(full_path) - 1] = '\0';
        }
        else
        {
            snprintf(full_path, sizeof(full_path), "%s/%s", source_dir, src);
        }

        /* Derive .o output path in build_dir */
        const char *base = strrchr(full_path, '/');
#ifdef _WIN32
        const char *base_bs = strrchr(full_path, '\\');
        if (base_bs && (!base || base_bs > base)) base = base_bs;
#endif
        base = base ? base + 1 : full_path;
        char o_path[PATH_MAX];
        const char *dot = strrchr(base, '.');
        int blen = dot ? (int)(dot - base) : (int)strlen(base);
        snprintf(o_path, sizeof(o_path), "%s" SN_PATH_SEP_STR "pragma_%.*s.o", build_dir, blen, base);

        snprintf(command, sizeof(command),
            "%s%s%s -c %s -w -Werror=implicit-function-declaration -std=%s -D_GNU_SOURCE %s "
            "-include \"%s/sn_types.h\" "
            "-I\"%s\" -I\"%s\" %s %s "
            "\"%s\" -o \"%s\" 2>\"%s\"",
            cc_quote, config->cc, cc_quote, mode_cflags, config->std, config->cflags,
            build_dir,
            build_dir, include_dir, deps_include_opt, pkg_include_opt,
            full_path, o_path, error_file);

        if (!run_compile_cmd(command, error_file, verbose))
        {
            fprintf(stderr, "Error: failed to compile pragma source %s\n", full_path);
            for (int j = 0; j < obj_count; j++) free(obj_files[j]);
            return false;
        }

        obj_files[obj_count] = strdup(o_path);
        obj_count++;
    }

    /* ---- Step 3: Link all .o files ---- */
    char extra_libs[PATH_MAX];
    extra_libs[0] = '\0';
    if (link_libs && link_lib_count > 0)
    {
        int offset = 0;
        for (int i = 0; i < link_lib_count && offset < (int)sizeof(extra_libs) - 8; i++)
        {
            const char *lib = link_libs[i];
            const char *override = get_ldlibs_for_lib(lib);
            if (override)
            {
                /* SN_LDLIBS_<name> is a full replacement — empty means suppress */
                if (override[0])
                {
                    int written = snprintf(extra_libs + offset, sizeof(extra_libs) - offset, " %s", override);
                    if (written > 0) offset += written;
                }
            }
            else
            {
                /* No config entry: default to -l<name> with platform lib name translation */
                const char *translated = translate_lib_name(lib);
                int written = snprintf(extra_libs + offset, sizeof(extra_libs) - offset, " -l%s", translated);
                if (written > 0) offset += written;
            }
        }
    }

    /* Build the link command with all .o files */
    char all_objs[8192];
    int obj_offset = 0;
    for (int i = 0; i < obj_count && obj_offset < (int)sizeof(all_objs) - PATH_MAX; i++)
    {
        int written = snprintf(all_objs + obj_offset, sizeof(all_objs) - obj_offset, " \"%s\"", obj_files[i]);
        if (written > 0) obj_offset += written;
    }

    char exe_path[PATH_MAX];
    strncpy(exe_path, output_exe, sizeof(exe_path) - 1);
    exe_path[sizeof(exe_path) - 1] = '\0';
    normalize_path_separators(exe_path);

    snprintf(command, sizeof(command),
        "%s%s%s %s -w -Werror=implicit-function-declaration -std=%s -D_GNU_SOURCE %s "
        "%s \"%s\" "
        "%s %s%s %s %s -o \"%s\" 2>\"%s\"",
        cc_quote, config->cc, cc_quote, mode_cflags, config->std, config->cflags,
        all_objs, runtime_lib,
        deps_lib_opt, pkg_lib_opt, extra_libs, config->ldlibs, config->ldflags,
        exe_path, error_file);

    bool link_ok = run_compile_cmd(command, error_file, verbose);

    /* Free obj_files */
    for (int i = 0; i < obj_count; i++) free(obj_files[i]);

    if (!link_ok)
    {
        fprintf(stderr, "Error: linking failed\n");
        return false;
    }

    if (verbose)
    {
        DEBUG_INFO("Successfully compiled (modular) to: %s", exe_path);
    }

    return true;
}
