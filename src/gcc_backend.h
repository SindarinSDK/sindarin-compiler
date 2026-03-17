#ifndef GCC_BACKEND_H
#define GCC_BACKEND_H

#include <stdbool.h>

/* Configuration for the C compiler backend */
typedef struct {
    const char *cc;              /* SN_CC: Compiler command (default: "gcc") */
    const char *std;             /* SN_STD: C standard (default: "c99") */
    const char *debug_cflags;    /* SN_DEBUG_CFLAGS: Debug mode flags */
    const char *release_cflags;  /* SN_RELEASE_CFLAGS: Release mode flags */
    const char *profile_cflags;  /* SN_PROFILE_CFLAGS: Profile mode flags */
    const char *cflags;          /* SN_CFLAGS: Additional compiler flags */
    const char *ldflags;         /* SN_LDFLAGS: Additional linker flags */
    const char *ldlibs;          /* SN_LDLIBS: Additional libraries */
} CCBackendConfig;

/* Load backend configuration from config file (if it exists) */
void cc_backend_load_config(const char *compiler_dir);

/* Initialize backend configuration from environment variables */
void cc_backend_init_config(CCBackendConfig *config);

/* Check if the C compiler is available on the system */
bool gcc_check_available(const CCBackendConfig *config, bool verbose);

/* Compile multiple C files and link into an executable.
 *
 *   build_dir      - Directory containing sn_types.h and all .c files
 *   c_files        - Array of .c file basenames (relative to build_dir)
 *   c_file_count   - Number of .c files
 *   output_exe     - Path for the output executable
 *   compiler_dir   - Compiler executable directory (SDK root)
 *   link_libs      - Array of library names from #pragma link
 *   pragma_sources - Array of .sn.c file paths from #pragma source
 *   pragma_dirs    - Array of source directories for resolving relative paths
 */
bool gcc_compile_modular(const CCBackendConfig *config, const char *build_dir,
                          const char **c_files, int c_file_count,
                          const char *output_exe, const char *compiler_dir,
                          bool verbose, bool debug_mode, bool profile_mode,
                          char **link_libs, int link_lib_count,
                          char **pragma_sources, char **pragma_dirs, int pragma_count);

/* Get the directory containing the compiler executable */
const char *gcc_get_compiler_dir(const char *argv0);

/* Resolve compiler_dir to the actual SDK root (where templates/ lives).
 * Handles the case where the binary is in bin/ but templates are in ../lib/sindarin/. */
void gcc_resolve_compiler_dir(char *dir_buf, int buf_size);

/* Resolve an SDK import to its full file path */
const char *gcc_resolve_sdk_import(const char *compiler_dir, const char *module_name);

/* Reset the SDK directory cache (for testing purposes) */
void gcc_reset_sdk_cache(void);

#endif
