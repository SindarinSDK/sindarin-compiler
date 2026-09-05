#ifndef SN_CC_SIDECAR_H
#define SN_CC_SIDECAR_H

#include "gcc_backend.h"
#include "gcc_backend_config.h"
#include <stdbool.h>

typedef struct {
    const char *build_dir;
    const char *compiler_dir;
    const char *const *generated_sources;
    int generated_source_count;
    const char *const *native_sources;
    const char *const *native_source_dirs;
    int native_source_count;
    const char *const *link_libraries;
    int link_library_count;
    bool verbose;
    bool debug_mode;
    bool profile_mode;
} CCSidecarBuildRequest;

/* Owned C object and native-link inputs. Raw option fragments retain the
 * configured C linker syntax; consumers must not reinterpret @link entries. */
typedef struct {
    BackendType backend;
    char *sdk_root;
    char *runtime_archive;
    char *mode_cflags;

    char **include_paths;
    int include_path_count;
    char **library_search_paths;
    int library_search_path_count;
    char **runtime_search_paths;
    int runtime_search_path_count;

    char *package_compile_options;
    char *package_link_options;
    /* One raw C-linker fragment per non-suppressed @link entry, in order. */
    char **requested_link_options;
    int requested_link_option_count;
    /* Exact joined form consumed by the existing C executable linker. */
    char *link_library_options;
    char *configured_libraries;
    char *configured_linker_options;

    char **object_files;
    int object_file_count;
} CCSidecarBuildPlan;

void cc_sidecar_build_plan_init(CCSidecarBuildPlan *plan);
void cc_sidecar_build_plan_free(CCSidecarBuildPlan *plan);

/* Resolve the established C compile/link environment and compile all supplied
 * generated/native C translation units to objects. This does not link an
 * executable. On success, out_plan owns every returned path and option. */
bool cc_sidecar_build(const CCBackendConfig *config,
                      const CCSidecarBuildRequest *request,
                      CCSidecarBuildPlan *out_plan);

#endif
