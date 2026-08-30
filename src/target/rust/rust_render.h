#ifndef SN_RUST_RENDER_H
#define SN_RUST_RENDER_H

#include <json-c/json.h>

char *rust_render_model(json_object *model, const char *template_dir);

#endif
