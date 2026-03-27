#ifndef FORMATTER_H
#define FORMATTER_H

/* Format all .sn files recursively from the given directory.
 * Skips .sn/ directories (package cache / build dir).
 * If check_only is true, don't modify files — just report which would change.
 * Returns the number of files formatted (or that would change), or -1 on error. */
int formatter_format_directory(const char *dir, int check_only);

/* Format a single .sn file.
 * Returns: 0 if unchanged, 1 if reformatted, -1 on error. */
int formatter_format_file(const char *path, int check_only);

#endif
