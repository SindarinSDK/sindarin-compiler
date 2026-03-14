#ifndef GEN_MODEL_SPLIT_H
#define GEN_MODEL_SPLIT_H

#include <json-c/json.h>

/* Result of splitting a unified model into per-source-file modules */
typedef struct ModularModel {
    json_object *common_header;     /* structs + forward decls + extern globals */
    json_object **impl_models;      /* per-source-file implementation models */
    const char **impl_names;        /* base names for filenames (e.g., "server", "router") */
    int impl_count;                 /* number of implementation files */

    /* Pragma extraction */
    char **link_libs;               /* -l library names from #pragma link */
    int link_lib_count;
    char **source_files;            /* .sn.c pragma source paths */
    char **source_dirs;             /* directory context for each pragma source */
    int source_file_count;
} ModularModel;

/* Split a unified JSON model into per-source-file modules.
 * The input model must have source_file fields on functions, globals, and structs.
 * entry_file is the main .sn source file path (used to identify the main module).
 * Returns a ModularModel, or NULL on error.
 * Caller must free with modular_model_free(). */
ModularModel *gen_model_split(json_object *model, const char *entry_file);

/* Free a ModularModel and all its contents. */
void modular_model_free(ModularModel *m);

#endif
