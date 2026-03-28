// parser_import_util.c
// Import path helper functions

/* Helper function to normalize a path by removing redundant ./ and resolving ../
 * This ensures paths like "a/./b/./c.sn" and "a/b/c.sn" are treated as the same,
 * and "a/b/../c.sn" resolves to "a/c.sn". */
static char *normalize_path(Arena *arena, const char *path)
{
    size_t len = strlen(path);
    /* Split path into components, then rebuild without . and resolving .. */
    /* Max components = len (one char per component in worst case) */
    const char **components = arena_alloc(arena, sizeof(const char *) * (len + 1));
    int *comp_lens = arena_alloc(arena, sizeof(int) * (len + 1));
    if (!components || !comp_lens) return NULL;
    int comp_count = 0;

    const char *p = path;
    while (*p) {
        /* Skip separators */
        if (*p == '/' || *p == '\\') { p++; continue; }
        /* Find end of component */
        const char *start = p;
        while (*p && *p != '/' && *p != '\\') p++;
        int clen = (int)(p - start);

        if (clen == 1 && start[0] == '.') {
            /* Skip "." component */
            continue;
        } else if (clen == 2 && start[0] == '.' && start[1] == '.') {
            /* ".." — pop the last component if possible */
            if (comp_count > 0 &&
                !(comp_lens[comp_count-1] == 2 && components[comp_count-1][0] == '.' && components[comp_count-1][1] == '.')) {
                comp_count--;
            } else {
                /* Can't pop (at root or previous is also ..) — keep it */
                components[comp_count] = start;
                comp_lens[comp_count] = clen;
                comp_count++;
            }
        } else {
            components[comp_count] = start;
            comp_lens[comp_count] = clen;
            comp_count++;
        }
    }

    /* Rebuild path */
    char *result = arena_alloc(arena, len + 1);
    if (!result) return NULL;
    size_t j = 0;
    for (int i = 0; i < comp_count; i++) {
        if (i > 0) result[j++] = '/';
        memcpy(result + j, components[i], comp_lens[i]);
        j += comp_lens[i];
    }
    result[j] = '\0';

    return result;
}

/* Helper function to construct the import path from current file and module name */
static char *construct_import_path(Arena *arena, const char *current_file, const char *module_name)
{
    /* Find the last path separator (handle both Unix '/' and Windows '\') */
    const char *dir_end_fwd = strrchr(current_file, '/');
    const char *dir_end_back = strrchr(current_file, '\\');
    const char *dir_end = dir_end_fwd;
    if (dir_end_back && (!dir_end || dir_end_back > dir_end)) {
        dir_end = dir_end_back;
    }
    size_t dir_len = dir_end ? (size_t)(dir_end - current_file + 1) : 0;

    size_t mod_name_len = strlen(module_name);
    size_t path_len = dir_len + mod_name_len + 4; /* +4 for ".sn\0" */
    char *import_path = arena_alloc(arena, path_len);
    if (!import_path) {
        return NULL;
    }

    if (dir_len > 0) {
        strncpy(import_path, current_file, dir_len);
        import_path[dir_len] = '\0';
    } else {
        import_path[0] = '\0';
    }
    strcat(import_path, module_name);
    strcat(import_path, ".sn");

    /* Normalize the path to remove redundant ./ components */
    return normalize_path(arena, import_path);
}

/* Helper: check if file exists */
static bool import_file_exists(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

/* Walk up the directory hierarchy from current_file, looking for .sn/<module_name>.sn.
 * This enables package-scoped imports: import "sindarin-pkg-sdk/src/net/tcp" resolves
 * to the nearest ancestor's .sn/sindarin-pkg-sdk/src/net/tcp.sn. */
static char *resolve_package_import(Arena *arena, const char *current_file, const char *module_name)
{
    size_t mod_name_len = strlen(module_name);
    size_t pos = strlen(current_file);

    /* Find end of directory portion (last path separator) */
    while (pos > 0 && current_file[pos - 1] != '/' && current_file[pos - 1] != '\\') {
        pos--;
    }

    for (;;) {
        /* Try: current_file[0..pos] + ".sn/" + module_name + ".sn" */
        size_t candidate_len = pos + mod_name_len + 8 + 1; /* ".sn/" + ".sn" + '\0' */
        char *candidate = arena_alloc(arena, candidate_len);
        if (!candidate) return NULL;
        strncpy(candidate, current_file, pos);
        candidate[pos] = '\0';
        strcat(candidate, ".sn/");
        strcat(candidate, module_name);
        strcat(candidate, ".sn");
        if (import_file_exists(candidate)) {
            return candidate;
        }

        if (pos == 0) break; /* Tried empty prefix (CWD), nothing more to walk up */

        /* Walk up one directory level (skip trailing separator, then skip dir name) */
        pos--;
        while (pos > 0 && current_file[pos - 1] != '/' && current_file[pos - 1] != '\\') {
            pos--;
        }
    }

    return NULL;
}
