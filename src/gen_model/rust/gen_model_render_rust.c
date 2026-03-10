#include "gen_model/gen_model_render.h"
#include "gen_model/gen_model_render_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Rust-Specific Helpers ---- */

static char *helper_rust_type(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) return strdup("()");

    json_object *type_obj = params[0];

    if (json_object_is_type(type_obj, json_type_string)) {
        return strdup(json_object_get_string(type_obj));
    }

    json_object *kind_obj = NULL;
    if (!json_object_object_get_ex(type_obj, "kind", &kind_obj)) return strdup("()");

    const char *kind = json_object_get_string(kind_obj);
    if (!kind) return strdup("()");

    if (strcmp(kind, "int") == 0) return strdup("i64");
    if (strcmp(kind, "long") == 0) return strdup("i64");
    if (strcmp(kind, "int32") == 0) return strdup("i32");
    if (strcmp(kind, "uint") == 0) return strdup("u64");
    if (strcmp(kind, "uint32") == 0) return strdup("u32");
    if (strcmp(kind, "double") == 0) return strdup("f64");
    if (strcmp(kind, "float") == 0) return strdup("f32");
    if (strcmp(kind, "bool") == 0) return strdup("bool");
    if (strcmp(kind, "char") == 0) return strdup("char");
    if (strcmp(kind, "byte") == 0) return strdup("u8");
    if (strcmp(kind, "string") == 0) return strdup("String");
    if (strcmp(kind, "void") == 0) return strdup("()");

    if (strcmp(kind, "array") == 0) {
        json_object *elem_obj = NULL;
        if (json_object_object_get_ex(type_obj, "element_type", &elem_obj)) {
            char *elem = helper_rust_type(&elem_obj, 1, options);
            char buf[256];
            snprintf(buf, sizeof(buf), "Vec<%s>", elem);
            free(elem);
            return strdup(buf);
        }
        return strdup("Vec<i64>");
    }

    if (strcmp(kind, "pointer") == 0) {
        json_object *pointee_obj = NULL;
        if (json_object_object_get_ex(type_obj, "pointee_type", &pointee_obj)) {
            char *pointee = helper_rust_type(&pointee_obj, 1, options);
            char buf[256];
            snprintf(buf, sizeof(buf), "*mut %s", pointee);
            free(pointee);
            return strdup(buf);
        }
        return strdup("*mut ()");
    }

    if (strcmp(kind, "struct") == 0) {
        json_object *name_obj = NULL;
        if (json_object_object_get_ex(type_obj, "name", &name_obj)) {
            const char *sname = json_object_get_string(name_obj);
            if (sname) return strdup(sname);
        }
    }

    return strdup("()");
}

static char *helper_rust_default(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) return strdup("0");

    json_object *type_obj = params[0];
    json_object *kind_obj = NULL;
    if (!json_object_object_get_ex(type_obj, "kind", &kind_obj)) return strdup("0");

    const char *kind = json_object_get_string(kind_obj);
    if (!kind) return strdup("0");

    if (strcmp(kind, "int") == 0) return strdup("0_i64");
    if (strcmp(kind, "long") == 0) return strdup("0_i64");
    if (strcmp(kind, "int32") == 0) return strdup("0_i32");
    if (strcmp(kind, "uint") == 0) return strdup("0_u64");
    if (strcmp(kind, "uint32") == 0) return strdup("0_u32");
    if (strcmp(kind, "double") == 0) return strdup("0.0_f64");
    if (strcmp(kind, "float") == 0) return strdup("0.0_f32");
    if (strcmp(kind, "bool") == 0) return strdup("false");
    if (strcmp(kind, "char") == 0) return strdup("'\\0'");
    if (strcmp(kind, "byte") == 0) return strdup("0_u8");
    if (strcmp(kind, "string") == 0) return strdup("String::new()");
    if (strcmp(kind, "void") == 0) return strdup("()");
    if (strcmp(kind, "array") == 0) return strdup("Vec::new()");

    return strdup("0");
}

/* ---- Register All Rust Helpers ---- */

static void register_rust_helpers(hbs_env_t *env)
{
    hbs_register_helper(env, "eq", helper_eq);
    hbs_register_helper(env, "rust_type", helper_rust_type);
    hbs_register_helper(env, "rust_default", helper_rust_default);
    hbs_register_helper(env, "op_symbol", helper_op_symbol);
}

/* ---- Public API ---- */

char *gen_model_render_rust(json_object *model, const char *template_dir)
{
    return render_with_helpers(model, template_dir, register_rust_helpers, "rust");
}
