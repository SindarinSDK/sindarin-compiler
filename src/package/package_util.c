/* ==============================================================================
 * package_util.c - Package Manager Utility Functions
 * ==============================================================================
 * Helper functions for file and directory operations.
 * ============================================================================== */

/* Check if a file exists */
static bool file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

/* Check if a directory exists */
static bool dir_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

/* Create directory if it doesn't exist */
static bool ensure_dir(const char *path)
{
    if (dir_exists(path)) {
        return true;
    }

#ifdef _WIN32
    return _mkdir(path) == 0;
#else
    return mkdir(path, 0755) == 0;
#endif
}

/* Read a line from stdin, stripping newline */
static void read_line(char *buffer, size_t size, const char *default_val)
{
    if (fgets(buffer, (int)size, stdin) == NULL) {
        buffer[0] = '\0';
    }

    /* Strip newline */
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    }

    /* Use default if empty */
    if (len == 0 && default_val != NULL) {
        strncpy(buffer, default_val, size - 1);
        buffer[size - 1] = '\0';
    }
}

/* Get current directory name for default project name */
static void pkg_get_dirname(char *buffer, size_t size)
{
    char cwd[PKG_MAX_PATH_LEN];

#ifdef _WIN32
    if (_getcwd(cwd, sizeof(cwd)) == NULL) {
#else
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
#endif
        strncpy(buffer, "my-project", size - 1);
        buffer[size - 1] = '\0';
        return;
    }

    /* Find last path separator */
    const char *last_sep = strrchr(cwd, PATH_SEP);
    if (last_sep != NULL) {
        strncpy(buffer, last_sep + 1, size - 1);
    } else {
        strncpy(buffer, cwd, size - 1);
    }
    buffer[size - 1] = '\0';
}

/* Make a file writable (needed for deleting read-only files on Windows) */
static void make_writable(const char *path)
{
#ifdef _WIN32
    /* Remove read-only attribute on Windows */
    _chmod(path, _S_IREAD | _S_IWRITE);
#else
    /* Make file writable on Unix */
    chmod(path, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif
}

/* Recursively remove a directory and its contents */
static bool remove_directory_recursive(const char *path)
{
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return false;
    }

    struct dirent *entry;
    bool success = true;

    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[PKG_MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s%c%s", path, PATH_SEP, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                /* Recursively remove subdirectory */
                if (!remove_directory_recursive(full_path)) {
                    success = false;
                }
            } else {
                /* Make file writable before removing (handles read-only files in .git) */
                make_writable(full_path);
                /* Remove file */
                if (unlink(full_path) != 0) {
                    success = false;
                }
            }
        }
    }

    closedir(dir);

    /* Remove the directory itself */
    if (rmdir(path) != 0) {
        success = false;
    }

    return success;
}

/* Get user home directory */
static bool get_home_dir(char *out_path, size_t out_len)
{
#ifdef _WIN32
    const char *userprofile = getenv("USERPROFILE");
    if (userprofile != NULL) {
        strncpy(out_path, userprofile, out_len - 1);
        out_path[out_len - 1] = '\0';
        return true;
    }
    return false;
#else
    const char *home = getenv("HOME");
    if (home != NULL) {
        strncpy(out_path, home, out_len - 1);
        out_path[out_len - 1] = '\0';
        return true;
    }
    return false;
#endif
}

/* Recursively copy a directory and its contents */
static bool copy_directory_recursive(const char *src_path, const char *dest_path)
{
    /* Create destination directory */
    if (!ensure_dir(dest_path)) {
        return false;
    }

    DIR *dir = opendir(src_path);
    if (dir == NULL) {
        return false;
    }

    struct dirent *entry;
    bool success = true;

    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_full[PKG_MAX_PATH_LEN];
        char dest_full[PKG_MAX_PATH_LEN];
        snprintf(src_full, sizeof(src_full), "%s%c%s", src_path, PATH_SEP, entry->d_name);
        snprintf(dest_full, sizeof(dest_full), "%s%c%s", dest_path, PATH_SEP, entry->d_name);

        struct stat st;
        if (stat(src_full, &st) != 0) {
            success = false;
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            /* Recursively copy subdirectory */
            if (!copy_directory_recursive(src_full, dest_full)) {
                success = false;
            }
        } else {
            /* Copy file */
            FILE *src_file = fopen(src_full, "rb");
            if (src_file == NULL) {
                success = false;
                continue;
            }

            FILE *dest_file = fopen(dest_full, "wb");
            if (dest_file == NULL) {
                fclose(src_file);
                success = false;
                continue;
            }

            char buffer[8192];
            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
                if (fwrite(buffer, 1, bytes_read, dest_file) != bytes_read) {
                    success = false;
                    break;
                }
            }

            fclose(src_file);
            fclose(dest_file);

            /* Preserve executable permission on non-Windows */
#ifndef _WIN32
            if (st.st_mode & S_IXUSR) {
                chmod(dest_full, st.st_mode);
            }
#endif
        }
    }

    closedir(dir);
    return success;
}
