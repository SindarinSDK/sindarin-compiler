#ifndef GEN_MODEL_RENDER_H
#define GEN_MODEL_RENDER_H

#include <json-c/json.h>
#include "cgen/gen_model_render_internal.h"

/* Forward declaration */
typedef struct ModularModel ModularModel;

/* ---- Single-file rendering (--emit-c diagnostic mode) ---- */

char *gen_model_render_min_c(json_object *model, const char *template_dir);

/* ---- Modular rendering (default compilation) ---- */

typedef struct {
    char *header_code;          /* rendered sn_types.h */
    char **impl_codes;          /* rendered .c files */
    const char **impl_names;    /* base names for each */
    int impl_count;
} ModularRenderResult;

ModularRenderResult *gen_model_render_modular_min_c(
    ModularModel *model, const char *template_dir,
    register_helpers_fn register_fn);

void modular_render_result_free(ModularRenderResult *r);

register_helpers_fn gen_model_get_min_c_register_fn(void);

#endif
