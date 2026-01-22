#ifndef GCC_BACKEND_H
#define GCC_BACKEND_H

#include <stdbool.h>

/* Forward declare for pragma source validation */
typedef struct PragmaSourceInfo PragmaSourceInfo;

/* Configuration for the C compiler backend.
 * All fields are initialized from environment variables with sensible defaults.
 */
typedef struct {
    const char *cc;              /* SN_CC: Compiler command (default: "gcc") */
    const char *std;             /* SN_STD: C standard (default: "c99") */
    const char *debug_cflags;    /* SN_DEBUG_CFLAGS: Debug mode flags */
    const char *release_cflags;  /* SN_RELEASE_CFLAGS: Release mode flags */
    const char *cflags;          /* SN_CFLAGS: Additional compiler flags */
    const char *ldflags;         /* SN_LDFLAGS: Additional linker flags */
    const char *ldlibs;          /* SN_LDLIBS: Additional libraries */
} CCBackendConfig;

/* Load backend configuration from config file (if it exists).
 * Must be called BEFORE cc_backend_init_config() to take effect.
 * Config file location: $SN_SDK/sn.cfg (or <exe_dir>/sn.cfg in dev mode)
 * Config file format: KEY=VALUE (one per line), supports:
 *   SN_CC, SN_STD, SN_DEBUG_CFLAGS, SN_RELEASE_CFLAGS, SN_CFLAGS, SN_LDFLAGS, SN_LDLIBS
 */
void cc_backend_load_config(const char *compiler_dir);

/* Initialize backend configuration from environment variables.
 * Uses defaults for any unset variables:
 *   SN_CC            = "gcc"
 *   SN_STD           = "c99"
 *   SN_DEBUG_CFLAGS  = "-no-pie -fsanitize=address -fno-omit-frame-pointer -g"
 *   SN_RELEASE_CFLAGS = "-O3 -flto"
 *   SN_CFLAGS        = ""
 *   SN_LDFLAGS       = ""
 *   SN_LDLIBS        = ""
 * Priority: Environment variable > Config file value > Default
 */
void cc_backend_init_config(CCBackendConfig *config);

/* Check if the C compiler is available on the system.
 * Returns true if the compiler (from config->cc) is found and executable.
 * If verbose is true, prints diagnostic information about what was found.
 */
bool gcc_check_available(const CCBackendConfig *config, bool verbose);

/* Validate pragma source files before passing to C compiler.
 * Checks that all source files exist and are readable.
 * Paths are resolved relative to each pragma's defining module directory.
 *
 * Parameters:
 *   source_files      - Array of PragmaSourceInfo with file paths and source directories
 *   source_file_count - Number of source files
 *   verbose           - If true, print diagnostic information
 *
 * Returns true if all files exist, false if any are missing.
 */
bool gcc_validate_pragma_sources(PragmaSourceInfo *source_files, int source_file_count,
                                  bool verbose);

/* Compile a C source file to an executable using the configured C compiler.
 *
 * Parameters:
 *   config        - Backend configuration (compiler, flags, etc.)
 *   c_file        - Path to the C source file to compile
 *   output_exe    - Path for the output executable (if NULL, derives from c_file)
 *   compiler_dir  - Compiler executable directory (fallback SDK root when $SN_SDK not set)
 *   verbose       - If true, print the compiler command being executed
 *   debug_mode    - If true, use debug flags; otherwise use release flags
 *   link_libs     - Array of library names to link (e.g., "m", "pthread") or NULL
 *   link_lib_count - Number of libraries in link_libs
 *   source_files  - Array of PragmaSourceInfo with file paths and source directories
 *   source_file_count - Number of source files
 *
 * Returns true on success, false on failure.
 */
bool gcc_compile(const CCBackendConfig *config, const char *c_file,
                 const char *output_exe, const char *compiler_dir,
                 bool verbose, bool debug_mode,
                 char **link_libs, int link_lib_count,
                 PragmaSourceInfo *source_files, int source_file_count);

/* Get the directory containing the compiler executable.
 * Used as the default SDK root when $SN_SDK is not set.
 * Returns a statically allocated string (do not free).
 */
const char *gcc_get_compiler_dir(const char *argv0);

/* Resolve an SDK import to its full file path.
 * Given a module name (e.g., "math"), returns the full path to the SDK file
 * (e.g., "$SN_SDK/sdk/math.sn") if it exists.
 *
 * SDK root resolution:
 *   1. $SN_SDK environment variable (if set)
 *   2. Compiler executable's directory (portable/development mode)
 *
 * Parameters:
 *   compiler_dir - Directory containing the compiler executable (fallback for SN_SDK)
 *   module_name  - Name of the module to import (without .sn extension)
 *
 * Returns path to SDK file or NULL if not found.
 * The returned path is statically allocated - do not free.
 */
const char *gcc_resolve_sdk_import(const char *compiler_dir, const char *module_name);

/* Reset the SDK directory cache (for testing purposes) */
void gcc_reset_sdk_cache(void);

#endif
