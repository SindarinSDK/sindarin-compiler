// parser_import_util.c
// Import path helper functions

/* Helper function to normalize a path by removing redundant ./ and resolving ..
 * This ensures paths like "a/./b/./c.sn" and "a/b/c.sn" are treated as the same. */
static char *normalize_path(Arena *arena, const char *path)
{
    size_t len = strlen(path);
    char *result = arena_alloc(arena, len + 1);
    if (!result) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; ) {
        /* Check for "./" at start or after a separator */
        if ((i == 0 || path[i-1] == '/' || path[i-1] == '\\') &&
            path[i] == '.' && (path[i+1] == '/' || path[i+1] == '\\')) {
            /* Skip the "./" */
            i += 2;
            continue;
        }
        /* Check for "./" at end (just ".") - shouldn't happen but handle it */
        if ((i == 0 || path[i-1] == '/' || path[i-1] == '\\') &&
            path[i] == '.' && path[i+1] == '\0') {
            i++;
            continue;
        }
        result[j++] = path[i++];
    }
    result[j] = '\0';

    /* Remove trailing slash if present (unless it's the root) */
    if (j > 1 && (result[j-1] == '/' || result[j-1] == '\\')) {
        result[j-1] = '\0';
    }

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
