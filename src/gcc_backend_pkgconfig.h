#ifndef GCC_BACKEND_PKGCONFIG_H
#define GCC_BACKEND_PKGCONFIG_H

#include <stddef.h>
#include <stdbool.h>

/* Append an include path to the options buffer */
void append_include_path(char *pkg_include_opts, size_t inc_size, const char *path);

/* Append a define to the options buffer */
void append_define(char *pkg_include_opts, size_t inc_size, const char *define);

/* Scan pkgconfig directory and parse all .pc files */
void parse_pkgconfig_dir(const char *pkgconfig_dir,
                         char *pkg_include_opts, size_t inc_size);

/* Build package library include/lib paths from sn.yaml dependencies.
 * Appends -I and -L/-rpath flags to the provided buffers.
 * Returns true if any package paths were added. */
bool build_package_lib_paths(char *pkg_include_opts, size_t inc_size,
                             char *pkg_lib_opts, size_t lib_size);

#endif /* GCC_BACKEND_PKGCONFIG_H */
