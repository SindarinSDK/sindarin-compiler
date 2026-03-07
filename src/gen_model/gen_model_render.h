#ifndef GEN_MODEL_RENDER_H
#define GEN_MODEL_RENDER_H

#include <json-c/json.h>

/* Render a JSON model to C code using Handlebars templates.
 * template_dir: path to directory containing templates (e.g., "bin/templates/c")
 * Returns a malloc'd string with the rendered C code, or NULL on error.
 * Caller must free() the returned string. */
char *gen_model_render_c(json_object *model, const char *template_dir);

#endif
