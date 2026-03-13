#include "gen_model/gen_model_render.h"
#include "gen_model/gen_model_render_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- C-Specific Helpers ---- */

static char *helper_c_type(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) {
        return strdup("void");
    }

    json_object *type_obj = params[0];

    if (json_object_is_type(type_obj, json_type_string)) {
        return strdup(json_object_get_string(type_obj));
    }

    json_object *kind_obj = NULL;
    if (!json_object_object_get_ex(type_obj, "kind", &kind_obj)) {
        return strdup("void");
    }

    const char *kind = json_object_get_string(kind_obj);
    if (!kind) return strdup("void");

    if (strcmp(kind, "int") == 0) return strdup("long long");
    if (strcmp(kind, "long") == 0) return strdup("long long");
    if (strcmp(kind, "double") == 0) return strdup("double");
    if (strcmp(kind, "float") == 0) return strdup("float");
    if (strcmp(kind, "bool") == 0) return strdup("bool");
    if (strcmp(kind, "char") == 0) return strdup("char");
    if (strcmp(kind, "byte") == 0) return strdup("unsigned char");
    if (strcmp(kind, "string") == 0) return strdup("RtHandleV2 *");
    if (strcmp(kind, "void") == 0) return strdup("void");
    if (strcmp(kind, "array") == 0) return strdup("RtHandleV2 *");
    if (strcmp(kind, "function") == 0) return strdup("RtHandleV2 *");

    if (strcmp(kind, "struct") == 0) {
        json_object *name_obj = NULL;
        if (json_object_object_get_ex(type_obj, "name", &name_obj)) {
            const char *sname = json_object_get_string(name_obj);
            if (sname) {
                char buf[256];
                snprintf(buf, sizeof(buf), "__sn__%s", sname);
                return strdup(buf);
            }
        }
    }

    return strdup("void");
}

static char *helper_default_value(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) return strdup("0");

    json_object *type_obj = params[0];
    json_object *kind_obj = NULL;
    if (!json_object_object_get_ex(type_obj, "kind", &kind_obj)) return strdup("0");

    const char *kind = json_object_get_string(kind_obj);
    if (!kind) return strdup("0");

    if (strcmp(kind, "int") == 0) return strdup("0");
    if (strcmp(kind, "long") == 0) return strdup("0");
    if (strcmp(kind, "double") == 0) return strdup("0.0");
    if (strcmp(kind, "float") == 0) return strdup("0.0f");
    if (strcmp(kind, "bool") == 0) return strdup("false");
    if (strcmp(kind, "char") == 0) return strdup("'\\0'");
    if (strcmp(kind, "byte") == 0) return strdup("0");
    if (strcmp(kind, "string") == 0) return strdup("NULL");
    if (strcmp(kind, "array") == 0) return strdup("NULL");
    if (strcmp(kind, "function") == 0) return strdup("NULL");
    if (strcmp(kind, "void") == 0) return strdup("");
    return strdup("0");
}

static char *helper_type_suffix(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) return strdup("long");

    json_object *type_obj = params[0];
    json_object *kind_obj = NULL;
    if (!json_object_object_get_ex(type_obj, "kind", &kind_obj)) return strdup("long");

    const char *kind = json_object_get_string(kind_obj);
    if (!kind) return strdup("long");

    if (strcmp(kind, "int") == 0) return strdup("long");
    if (strcmp(kind, "long") == 0) return strdup("long");
    if (strcmp(kind, "int32") == 0) return strdup("int32");
    if (strcmp(kind, "uint") == 0) return strdup("uint");
    if (strcmp(kind, "uint32") == 0) return strdup("uint32");
    if (strcmp(kind, "double") == 0) return strdup("double");
    if (strcmp(kind, "float") == 0) return strdup("float");
    if (strcmp(kind, "char") == 0) return strdup("char");
    if (strcmp(kind, "bool") == 0) return strdup("bool");
    if (strcmp(kind, "byte") == 0) return strdup("byte");
    if (strcmp(kind, "string") == 0) return strdup("string");
    if (strcmp(kind, "array") == 0) return strdup("handle");

    return strdup("long");
}

static char *helper_c_sizeof(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) return strdup("sizeof(long long)");

    json_object *type_obj = params[0];
    json_object *kind_obj = NULL;
    if (!json_object_object_get_ex(type_obj, "kind", &kind_obj)) return strdup("sizeof(long long)");

    const char *kind = json_object_get_string(kind_obj);
    if (!kind) return strdup("sizeof(long long)");

    if (strcmp(kind, "int") == 0) return strdup("sizeof(long long)");
    if (strcmp(kind, "long") == 0) return strdup("sizeof(long long)");
    if (strcmp(kind, "int32") == 0) return strdup("sizeof(int32_t)");
    if (strcmp(kind, "uint") == 0) return strdup("sizeof(uint64_t)");
    if (strcmp(kind, "uint32") == 0) return strdup("sizeof(uint32_t)");
    if (strcmp(kind, "double") == 0) return strdup("sizeof(double)");
    if (strcmp(kind, "float") == 0) return strdup("sizeof(float)");
    if (strcmp(kind, "char") == 0) return strdup("sizeof(char)");
    if (strcmp(kind, "bool") == 0) return strdup("sizeof(int)");
    if (strcmp(kind, "byte") == 0) return strdup("sizeof(unsigned char)");
    if (strcmp(kind, "string") == 0) return strdup("sizeof(RtHandleV2 *)");
    if (strcmp(kind, "array") == 0) return strdup("sizeof(RtHandleV2 *)");

    if (strcmp(kind, "struct") == 0) {
        json_object *name_obj = NULL;
        if (json_object_object_get_ex(type_obj, "name", &name_obj)) {
            const char *sname = json_object_get_string(name_obj);
            if (sname) {
                char buf[256];
                snprintf(buf, sizeof(buf), "sizeof(__sn__%s)", sname);
                return strdup(buf);
            }
        }
    }

    return strdup("sizeof(long long)");
}

static char *helper_to_string_func(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) return strdup("rt_to_string_long_v2");

    json_object *type_obj = params[0];
    json_object *kind_obj = NULL;
    if (!json_object_object_get_ex(type_obj, "kind", &kind_obj)) return strdup("rt_to_string_long_v2");

    const char *kind = json_object_get_string(kind_obj);
    if (!kind) return strdup("rt_to_string_long_v2");

    if (strcmp(kind, "int") == 0) return strdup("rt_to_string_long_v2");
    if (strcmp(kind, "long") == 0) return strdup("rt_to_string_long_v2");
    if (strcmp(kind, "int32") == 0) return strdup("rt_to_string_long_v2");
    if (strcmp(kind, "uint") == 0) return strdup("rt_to_string_long_v2");
    if (strcmp(kind, "uint32") == 0) return strdup("rt_to_string_long_v2");
    if (strcmp(kind, "double") == 0) return strdup("rt_to_string_double_v2");
    if (strcmp(kind, "float") == 0) return strdup("rt_to_string_double_v2");
    if (strcmp(kind, "char") == 0) return strdup("rt_to_string_char_v2");
    if (strcmp(kind, "bool") == 0) return strdup("rt_to_string_bool_v2");
    if (strcmp(kind, "byte") == 0) return strdup("rt_to_string_byte_v2");
    if (strcmp(kind, "string") == 0) return strdup("__string__");

    return strdup("rt_to_string_long_v2");
}

static char *helper_type_tag(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) return strdup("RT_ANY_INT");

    json_object *type_obj = params[0];
    json_object *kind_obj = NULL;
    if (!json_object_object_get_ex(type_obj, "kind", &kind_obj)) return strdup("RT_ANY_INT");

    const char *kind = json_object_get_string(kind_obj);
    if (!kind) return strdup("RT_ANY_INT");

    if (strcmp(kind, "int") == 0) return strdup("RT_ANY_INT");
    if (strcmp(kind, "long") == 0) return strdup("RT_ANY_LONG");
    if (strcmp(kind, "double") == 0) return strdup("RT_ANY_DOUBLE");
    if (strcmp(kind, "float") == 0) return strdup("RT_ANY_FLOAT");
    if (strcmp(kind, "bool") == 0) return strdup("RT_ANY_BOOL");
    if (strcmp(kind, "char") == 0) return strdup("RT_ANY_CHAR");
    if (strcmp(kind, "byte") == 0) return strdup("RT_ANY_BYTE");
    if (strcmp(kind, "string") == 0) return strdup("RT_ANY_STRING");
    if (strcmp(kind, "int32") == 0) return strdup("RT_ANY_INT32");
    if (strcmp(kind, "uint") == 0) return strdup("RT_ANY_UINT");
    if (strcmp(kind, "uint32") == 0) return strdup("RT_ANY_UINT32");
    if (strcmp(kind, "array") == 0) return strdup("RT_ANY_ARRAY");

    return strdup("RT_ANY_INT");
}

static char *helper_box_func(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) return strdup("rt_box_int");

    json_object *type_obj = params[0];
    json_object *kind_obj = NULL;
    if (!json_object_object_get_ex(type_obj, "kind", &kind_obj)) return strdup("rt_box_int");

    const char *kind = json_object_get_string(kind_obj);
    if (!kind) return strdup("rt_box_int");

    if (strcmp(kind, "int") == 0) return strdup("rt_box_int");
    if (strcmp(kind, "long") == 0) return strdup("rt_box_long");
    if (strcmp(kind, "double") == 0) return strdup("rt_box_double");
    if (strcmp(kind, "float") == 0) return strdup("rt_box_float");
    if (strcmp(kind, "bool") == 0) return strdup("rt_box_bool");
    if (strcmp(kind, "char") == 0) return strdup("rt_box_char");
    if (strcmp(kind, "byte") == 0) return strdup("rt_box_byte");
    if (strcmp(kind, "string") == 0) return strdup("rt_box_string");
    if (strcmp(kind, "int32") == 0) return strdup("rt_box_int32");
    if (strcmp(kind, "uint") == 0) return strdup("rt_box_uint");
    if (strcmp(kind, "uint32") == 0) return strdup("rt_box_uint32");
    if (strcmp(kind, "array") == 0) return strdup("rt_box_array");

    return strdup("rt_box_int");
}

static char *helper_unbox_func(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) return strdup("rt_unbox_int");

    json_object *type_obj = params[0];
    json_object *kind_obj = NULL;
    if (!json_object_object_get_ex(type_obj, "kind", &kind_obj)) return strdup("rt_unbox_int");

    const char *kind = json_object_get_string(kind_obj);
    if (!kind) return strdup("rt_unbox_int");

    if (strcmp(kind, "int") == 0) return strdup("rt_unbox_int");
    if (strcmp(kind, "long") == 0) return strdup("rt_unbox_long");
    if (strcmp(kind, "double") == 0) return strdup("rt_unbox_double");
    if (strcmp(kind, "float") == 0) return strdup("rt_unbox_float");
    if (strcmp(kind, "bool") == 0) return strdup("rt_unbox_bool");
    if (strcmp(kind, "char") == 0) return strdup("rt_unbox_char");
    if (strcmp(kind, "byte") == 0) return strdup("rt_unbox_byte");
    if (strcmp(kind, "string") == 0) return strdup("rt_unbox_string");
    if (strcmp(kind, "int32") == 0) return strdup("rt_unbox_int32");
    if (strcmp(kind, "uint") == 0) return strdup("rt_unbox_uint");
    if (strcmp(kind, "uint32") == 0) return strdup("rt_unbox_uint32");
    if (strcmp(kind, "array") == 0) return strdup("rt_unbox_array");

    return strdup("rt_unbox_int");
}

/* ---- Register All C Helpers ---- */

static void register_c_helpers(hbs_env_t *env)
{
    hbs_register_helper(env, "eq", helper_eq);
    hbs_register_helper(env, "c_type", helper_c_type);
    hbs_register_helper(env, "default_value", helper_default_value);
    hbs_register_helper(env, "type_suffix", helper_type_suffix);
    hbs_register_helper(env, "c_sizeof", helper_c_sizeof);
    hbs_register_helper(env, "to_string_func", helper_to_string_func);
    hbs_register_helper(env, "op_symbol", helper_op_symbol);
    hbs_register_helper(env, "return_label", helper_return_label);
    hbs_register_helper(env, "type_tag", helper_type_tag);
    hbs_register_helper(env, "box_func", helper_box_func);
    hbs_register_helper(env, "unbox_func", helper_unbox_func);
    hbs_register_helper(env, "count", helper_count);
}

/* ---- Public API ---- */

char *gen_model_render_c(json_object *model, const char *template_dir)
{
    return render_with_helpers(model, template_dir, register_c_helpers, "c");
}
