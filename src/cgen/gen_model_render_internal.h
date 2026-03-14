#ifndef GEN_MODEL_RENDER_INTERNAL_H
#define GEN_MODEL_RENDER_INTERNAL_H

#include "handlebars.h"
#include <json-c/json.h>

/* ---- Shared Helpers (used by all backends) ---- */

char *helper_eq(json_object **params, int param_count, hbs_options_t *options);
char *helper_op_symbol(json_object **params, int param_count, hbs_options_t *options);
char *helper_count(json_object **params, int param_count, hbs_options_t *options);
char *helper_return_label(json_object **params, int param_count, hbs_options_t *options);

/* ---- File I/O & Partial Registration ---- */

char *render_read_file(const char *path);
int render_register_partials_recursive(hbs_env_t *env, const char *base_dir,
                                       const char *rel_prefix);

/* ---- Common render scaffold ---- */

/* Creates an hbs environment, registers partials from template_dir/partials/,
 * compiles module.hbs, renders, and returns the result string.
 * register_helpers_fn is called to register backend-specific helpers on the env. */
typedef void (*register_helpers_fn)(hbs_env_t *env);
char *render_with_helpers(json_object *model, const char *template_dir,
                          register_helpers_fn register_fn, const char *backend_name);

#endif
