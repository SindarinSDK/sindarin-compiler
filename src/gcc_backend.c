#include "gcc_backend.h"
#include "gcc_backend_config.h"
#include "gcc_backend_pkgconfig.h"
#include "code_gen.h"
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

/* Default values for backend configuration */
#define DEFAULT_STD "c99"
#ifdef __APPLE__
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

/* Static buffers for config file values */
static char cfg_cc[256];
static char cfg_std[64];
static char cfg_debug_cflags[1024];
static char cfg_release_cflags[1024];
static char cfg_cflags[1024];
static char cfg_ldflags[1024];
static char cfg_ldlibs[1024];
static bool cfg_loaded = false;

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

void cc_backend_load_config(const char *compiler_dir)
{
    if (cfg_loaded) return;
    cfg_loaded = true;

    const char *sdk_root = get_sdk_root(compiler_dir);
    char config_path[PATH_MAX];
    snprintf(config_path, sizeof(config_path), "%s" SN_PATH_SEP_STR "sn.cfg", sdk_root);

    FILE *f = fopen(config_path, "r");
    if (!f) return;

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

bool gcc_validate_pragma_sources(PragmaSourceInfo *source_files, int source_file_count,
                                  bool verbose)
{
#ifdef _WIN32
    (void)source_files;
    (void)source_file_count;
    (void)verbose;
    return true;
#else
    if (source_files == NULL || source_file_count == 0)
        return true;

    bool all_valid = true;

    for (int i = 0; i < source_file_count; i++)
    {
        const char *src = source_files[i].value;
        const char *source_dir = source_files[i].source_dir;

        size_t len = strlen(src);
        char unquoted[PATH_MAX];
        if (len >= 2 && src[0] == '"' && src[len - 1] == '"')
        {
            strncpy(unquoted, src + 1, len - 2);
            unquoted[len - 2] = '\0';
            src = unquoted;
        }

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

    strncpy(c_file_normalized, c_file, sizeof(c_file_normalized) - 1);
    c_file_normalized[sizeof(c_file_normalized) - 1] = '\0';
    normalize_path_separators(c_file_normalized);

    BackendType backend = detect_backend(config->cc);
    const char *sdk_root = get_sdk_root(compiler_dir);

    snprintf(lib_dir, sizeof(lib_dir), "%s" SN_PATH_SEP_STR "%s", sdk_root, backend_lib_subdir(backend));
    snprintf(include_dir, sizeof(include_dir), "%s" SN_PATH_SEP_STR "include", sdk_root);

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

    if (verbose)
    {
        DEBUG_INFO("Using %s backend, lib_dir=%s", backend_name(backend), lib_dir);
    }

    if (output_exe == NULL || output_exe[0] == '\0')
    {
        strncpy(exe_path, c_file, sizeof(exe_path) - 1);
        exe_path[sizeof(exe_path) - 1] = '\0';

        char *dot = strrchr(exe_path, '.');
        if (dot != NULL && strcmp(dot, ".c") == 0)
        {
            *dot = '\0';
        }
    }
    else
    {
        strncpy(exe_path, output_exe, sizeof(exe_path) - 1);
        exe_path[sizeof(exe_path) - 1] = '\0';
    }

    normalize_path_separators(exe_path);

    snprintf(runtime_lib, sizeof(runtime_lib), "%s" SN_PATH_SEP_STR "libsn_runtime.a", lib_dir);

    if (backend != BACKEND_MSVC && access(runtime_lib, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime library not found: %s\n", runtime_lib);
        fprintf(stderr, "The '%s' backend runtime is not built.\n", backend_name(backend));
        fprintf(stderr, "Run 'make build' to build the runtime.\n");
        return false;
    }

    extra_libs[0] = '\0';
    if (link_libs != NULL && link_lib_count > 0)
    {
        int offset = 0;
        for (int i = 0; i < link_lib_count && offset < (int)sizeof(extra_libs) - 8; i++)
        {
            const char *lib = translate_lib_name(link_libs[i]);
            int written = snprintf(extra_libs + offset, sizeof(extra_libs) - offset, " -l%s", lib);
            if (written > 0)
            {
                offset += written;
            }
        }
    }

    /* Append transitive dependencies for statically-linked libraries */
    if (link_libs != NULL && link_lib_count > 0)
    {
        bool needs_openssl_deps = false;
        bool needs_ssh_deps = false;
        bool needs_git2_deps = false;

        for (int i = 0; i < link_lib_count; i++)
        {
            if (strcmp(link_libs[i], "ssl") == 0 || strcmp(link_libs[i], "crypto") == 0 ||
                strcmp(link_libs[i], "ngtcp2") == 0 || strcmp(link_libs[i], "ngtcp2_crypto_ossl") == 0)
            {
                needs_openssl_deps = true;
            }
            if (strcmp(link_libs[i], "ssh") == 0)
            {
                needs_ssh_deps = true;
            }
            if (strcmp(link_libs[i], "git2") == 0)
            {
                needs_git2_deps = true;
            }
        }

        int offset = (int)strlen(extra_libs);
        if (needs_openssl_deps)
        {
#ifdef _WIN32
            snprintf(extra_libs + offset, sizeof(extra_libs) - offset, " -lcrypt32");
#elif defined(__APPLE__)
            snprintf(extra_libs + offset, sizeof(extra_libs) - offset, " -framework Security -framework CoreFoundation");
#else
            snprintf(extra_libs + offset, sizeof(extra_libs) - offset, " -ldl");
#endif
            offset = (int)strlen(extra_libs);
        }

        if (needs_ssh_deps)
        {
#ifdef _WIN32
            snprintf(extra_libs + offset, sizeof(extra_libs) - offset, " -lzlib -lbcrypt -lws2_32 -liphlpapi");
#else
            snprintf(extra_libs + offset, sizeof(extra_libs) - offset, " -lz -lpthread");
#endif
            offset = (int)strlen(extra_libs);
        }

        if (needs_git2_deps)
        {
#ifdef _WIN32
            snprintf(extra_libs + offset, sizeof(extra_libs) - offset,
                " -lhttp_parser -lssh2 -lpcre2-8 -lzlib -lssl -lcrypto -lws2_32 -lsecur32 -lbcrypt -lcrypt32 -lrpcrt4 -lole32");
#elif defined(__APPLE__)
            snprintf(extra_libs + offset, sizeof(extra_libs) - offset,
                " -lhttp_parser -lssh2 -lpcre2-8 -lz -lssl -lcrypto -liconv -framework Security -framework CoreFoundation");
#else
            snprintf(extra_libs + offset, sizeof(extra_libs) - offset,
                " -lhttp_parser -lssh2 -lpcre2-8 -lz -lssl -lcrypto -lpthread -ldl");
#endif
        }
    }

    extra_sources[0] = '\0';
    if (source_files != NULL && source_file_count > 0)
    {
        int offset = 0;
        for (int i = 0; i < source_file_count && offset < (int)sizeof(extra_sources) - 256; i++)
        {
            const char *src = source_files[i].value;
            const char *source_dir = source_files[i].source_dir;

            size_t len = strlen(src);
            char unquoted[PATH_MAX];
            if (len >= 2 && src[0] == '"' && src[len - 1] == '"')
            {
                strncpy(unquoted, src + 1, len - 2);
                unquoted[len - 2] = '\0';
                src = unquoted;
            }

            char full_path[PATH_MAX];
            if (src[0] == '/' || (strlen(src) > 1 && src[1] == ':'))
            {
                strncpy(full_path, src, sizeof(full_path) - 1);
                full_path[sizeof(full_path) - 1] = '\0';
            }
            else
            {
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
    {
        strcpy(error_file, "/tmp/sn_cc_errors.txt");
    }
    else
    {
        close(error_fd);
    }
#endif

    const char *mode_cflags = debug_mode ? config->debug_cflags : config->release_cflags;
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
    {
        snprintf(deps_lib_opt, sizeof(deps_lib_opt), "-L\"%s\" -Wl,-rpath,\"%s\"", deps_lib_dir, deps_lib_dir);
    }
    else
        deps_lib_opt[0] = '\0';

    char pkg_include_opt[PATH_MAX * 4];
    char pkg_lib_opt[PATH_MAX * 4];
    build_package_lib_paths(pkg_include_opt, sizeof(pkg_include_opt),
                            pkg_lib_opt, sizeof(pkg_lib_opt));

    if (verbose && pkg_include_opt[0])
    {
        DEBUG_INFO("Package includes: %s", pkg_include_opt);
        DEBUG_INFO("Package libs: %s", pkg_lib_opt);
    }

    if (backend == BACKEND_MSVC)
    {
        char runtime_lib_msvc[PATH_MAX];
        snprintf(runtime_lib_msvc, sizeof(runtime_lib_msvc), "%s" SN_PATH_SEP_STR "sn_runtime.lib", lib_dir);

        if (access(runtime_lib_msvc, R_OK) != 0)
        {
            fprintf(stderr, "Error: Runtime library not found: %s\n", runtime_lib_msvc);
            fprintf(stderr, "The MSVC backend runtime is not built.\n");
            return false;
        }

        const char *cc_quote = (strchr(config->cc, ' ') != NULL) ? "\"" : "";
        char msvc_deps_opt[PATH_MAX + 8];
        if (deps_include_dir[0])
            snprintf(msvc_deps_opt, sizeof(msvc_deps_opt), "/I\"%s\"", deps_include_dir);
        else
            msvc_deps_opt[0] = '\0';

        snprintf(command, sizeof(command),
            "%s%s%s %s %s /I\"%s\" %s \"%s\"%s \"%s\" %s %s /Fe\"%s\" /link %s 2>\"%s\"",
            cc_quote, config->cc, cc_quote, mode_cflags, config->cflags, include_dir, msvc_deps_opt,
            c_file_normalized, extra_sources, runtime_lib_msvc, config->ldlibs, config->ldflags, exe_path,
            config->ldlibs, error_file);
    }
    else
    {
        const char *cc_quote = (strchr(config->cc, ' ') != NULL) ? "\"" : "";
#ifdef _WIN32
        snprintf(command, sizeof(command),
            "%s%s%s %s -w -std=%s -DSN_USE_WIN32_THREADS %s -I\"%s\" %s %s "
            "\"%s\"%s -Wl,--whole-archive \"%s\" -Wl,--no-whole-archive "
            "%s %s -lpthread -lm%s %s %s -o \"%s\" 2>\"%s\"",
            cc_quote, config->cc, cc_quote, mode_cflags, config->std, config->cflags, include_dir, deps_include_opt, pkg_include_opt,
            c_file_normalized, extra_sources, runtime_lib,
            deps_lib_opt, pkg_lib_opt, extra_libs, config->ldlibs, config->ldflags, exe_path, error_file);
#elif defined(__APPLE__)
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
    }

    if (verbose)
    {
        DEBUG_INFO("Executing: %s", command);
    }

    int result = system(command);

    if (result != 0)
    {
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

    unlink(error_file);

    if (verbose)
    {
        DEBUG_INFO("Successfully compiled to: %s", exe_path);
    }

    return true;
}
