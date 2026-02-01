#include "gcc_backend_pkgconfig.h"
#include "package.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "platform/platform.h"
#include <windows.h>
#define SN_PATH_SEP '\\'
#define SN_PATH_SEP_STR "\\"
#else
#include <dirent.h>
#include <sys/stat.h>
#define SN_PATH_SEP '/'
#define SN_PATH_SEP_STR "/"
#endif

#include <limits.h>

/* Helper: check if directory exists */
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

void append_include_path(char *pkg_include_opts, size_t inc_size, const char *path)
{
    size_t len = strlen(pkg_include_opts);
    if (len > 0) {
        snprintf(pkg_include_opts + len, inc_size - len, " -I\"%s\"", path);
    } else {
        snprintf(pkg_include_opts, inc_size, "-I\"%s\"", path);
    }
}

void append_define(char *pkg_include_opts, size_t inc_size, const char *define)
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

            /* Parse individual flags */
            char *p = expanded;
            while (*p) {
                /* Skip whitespace */
                while (*p == ' ' || *p == '\t') p++;
                if (!*p) break;

                /* Extract token - handle quoted tokens */
                char token[PATH_MAX];
                size_t tok_len = 0;
                if (*p == '"') {
                    p++; /* skip opening quote */
                    while (*p && *p != '"' && tok_len < sizeof(token) - 1) {
                        token[tok_len++] = *p++;
                    }
                    if (*p == '"') p++;
                } else {
                    while (*p && *p != ' ' && *p != '\t' && tok_len < sizeof(token) - 1) {
                        token[tok_len++] = *p++;
                    }
                }
                token[tok_len] = '\0';

                if (strncmp(token, "-I", 2) == 0) {
                    char clean_path[PATH_MAX];
                    strncpy(clean_path, token + 2, sizeof(clean_path) - 1);
                    clean_path[sizeof(clean_path) - 1] = '\0';
                    normalize_pc_path(clean_path);
                    if (clean_path[0] && dir_exists(clean_path)) {
                        append_include_path(pkg_include_opts, inc_size, clean_path);
                    }
                } else if (strncmp(token, "-D", 2) == 0) {
                    append_define(pkg_include_opts, inc_size, token);
                }
            }
        }
    }

    fclose(f);
}

void parse_pkgconfig_dir(const char *pkgconfig_dir,
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

bool build_package_lib_paths(char *pkg_include_opts, size_t inc_size,
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
            if (len > 0) {
                snprintf(pkg_lib_opts + len, lib_size - len, " -L\"%s\" -Wl,-rpath,\"%s\"", pkg_lib_dir, pkg_lib_dir);
            } else {
                snprintf(pkg_lib_opts, lib_size, "-L\"%s\" -Wl,-rpath,\"%s\"", pkg_lib_dir, pkg_lib_dir);
            }
            added = true;
        }
    }

    return added;
}
