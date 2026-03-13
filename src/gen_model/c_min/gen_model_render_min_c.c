#include "gen_model/gen_model_render.h"
#include "gen_model/gen_model_render_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Minimal C-Specific Helpers ---- */

/* Resolve a type JSON object to a C type string */
static char *resolve_c_type_min(json_object *type_obj)
{
    if (!type_obj) return strdup("void");

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
    if (strcmp(kind, "int32") == 0) return strdup("int32_t");
    if (strcmp(kind, "uint") == 0) return strdup("uint64_t");
    if (strcmp(kind, "uint32") == 0) return strdup("uint32_t");
    if (strcmp(kind, "double") == 0) return strdup("double");
    if (strcmp(kind, "float") == 0) return strdup("float");
    if (strcmp(kind, "bool") == 0) return strdup("bool");
    if (strcmp(kind, "char") == 0) return strdup("char");
    if (strcmp(kind, "byte") == 0) return strdup("unsigned char");
    if (strcmp(kind, "string") == 0) return strdup("char *");
    if (strcmp(kind, "void") == 0) return strdup("void");
    if (strcmp(kind, "any") == 0) return strdup("long long");  /* fallback */
    if (strcmp(kind, "array") == 0) return strdup("SnArray *");
    if (strcmp(kind, "function") == 0) return strdup("void *");

    if (strcmp(kind, "pointer") == 0) {
        json_object *base_obj = NULL;
        if (json_object_object_get_ex(type_obj, "base_type", &base_obj)) {
            char *base = resolve_c_type_min(base_obj);
            char buf[256];
            snprintf(buf, sizeof(buf), "%s *", base);
            free(base);
            return strdup(buf);
        }
        return strdup("void *");
    }

    if (strcmp(kind, "opaque") == 0) {
        return strdup("void *");
    }

    if (strcmp(kind, "struct") == 0) {
        json_object *name_obj = NULL;
        json_object *ref_obj = NULL;
        if (json_object_object_get_ex(type_obj, "name", &name_obj)) {
            const char *sname = json_object_get_string(name_obj);
            if (sname) {
                char buf[256];
                /* as ref structs are heap-allocated pointers */
                bool is_ref = false;
                if (json_object_object_get_ex(type_obj, "pass_self_by_ref", &ref_obj))
                    is_ref = json_object_get_boolean(ref_obj);
                if (is_ref)
                    snprintf(buf, sizeof(buf), "__sn__%s *", sname);
                else
                    snprintf(buf, sizeof(buf), "__sn__%s", sname);
                return strdup(buf);
            }
        }
    }

    return strdup("void");
}

/* Maps Sindarin types to minimal C types:
 * string -> char *  (not RtHandleV2 *)
 * array  -> SnArray *  (not RtHandleV2 *)
 * function -> void *  (not RtHandleV2 *)
 * any -> not supported */
static char *helper_c_type_min(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) {
        return strdup("void");
    }
    return resolve_c_type_min(params[0]);
}

static char *helper_default_value_min(json_object **params, int param_count, hbs_options_t *options)
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
    if (strcmp(kind, "pointer") == 0) return strdup("NULL");
    if (strcmp(kind, "opaque") == 0) return strdup("NULL");
    if (strcmp(kind, "void") == 0) return strdup("");
    if (strcmp(kind, "any") == 0) return strdup("0");

    /* Struct types: use {0} for aggregate initialization */
    return strdup("{0}");
}

static char *helper_type_suffix_min(json_object **params, int param_count, hbs_options_t *options)
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
    if (strcmp(kind, "array") == 0) return strdup("generic");

    return strdup("long");
}

static char *helper_c_sizeof_min(json_object **params, int param_count, hbs_options_t *options)
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
    if (strcmp(kind, "bool") == 0) return strdup("sizeof(bool)");
    if (strcmp(kind, "byte") == 0) return strdup("sizeof(unsigned char)");
    if (strcmp(kind, "any") == 0) return strdup("sizeof(long long)");
    if (strcmp(kind, "string") == 0) return strdup("sizeof(char *)");
    if (strcmp(kind, "array") == 0) return strdup("sizeof(SnArray *)");
    if (strcmp(kind, "pointer") == 0) return strdup("sizeof(void *)");
    if (strcmp(kind, "opaque") == 0) return strdup("sizeof(void *)");

    if (strcmp(kind, "struct") == 0) {
        json_object *name_obj = NULL;
        json_object *ref_obj = NULL;
        if (json_object_object_get_ex(type_obj, "name", &name_obj)) {
            const char *sname = json_object_get_string(name_obj);
            if (sname) {
                char buf[256];
                bool is_ref = false;
                if (json_object_object_get_ex(type_obj, "pass_self_by_ref", &ref_obj))
                    is_ref = json_object_get_boolean(ref_obj);
                if (is_ref)
                    snprintf(buf, sizeof(buf), "sizeof(__sn__%s *)", sname);
                else
                    snprintf(buf, sizeof(buf), "sizeof(__sn__%s)", sname);
                return strdup(buf);
            }
        }
    }

    return strdup("sizeof(long long)");
}

static char *helper_to_string_func_min(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    (void)param_count;
    (void)params;
    /* Minimal runtime doesn't have to_string functions — handled in templates */
    return strdup("/* to_string */");
}

static char *helper_type_tag_min(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) return strdup("0");

    json_object *type_obj = params[0];
    json_object *kind_obj = NULL;
    if (!json_object_object_get_ex(type_obj, "kind", &kind_obj)) return strdup("0");

    const char *kind = json_object_get_string(kind_obj);
    if (!kind) return strdup("0");

    /* Simple integer tags for minimal runtime */
    if (strcmp(kind, "int") == 0) return strdup("1");
    if (strcmp(kind, "long") == 0) return strdup("2");
    if (strcmp(kind, "double") == 0) return strdup("3");
    if (strcmp(kind, "float") == 0) return strdup("4");
    if (strcmp(kind, "bool") == 0) return strdup("5");
    if (strcmp(kind, "char") == 0) return strdup("6");
    if (strcmp(kind, "byte") == 0) return strdup("7");
    if (strcmp(kind, "string") == 0) return strdup("8");
    if (strcmp(kind, "array") == 0) return strdup("9");

    return strdup("0");
}

/* printf_format: given a Sindarin format spec (e.g. "05d") and a type, produce
 * the correct printf format string (e.g. "%05lld") */
static char *helper_printf_format(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 2 || !params[0] || !params[1]) return strdup("%lld");

    const char *spec = json_object_get_string(params[0]);
    if (!spec || !*spec) return strdup("%lld");

    json_object *type_obj = params[1];
    json_object *kind_obj = NULL;
    const char *kind = "";
    if (json_object_object_get_ex(type_obj, "kind", &kind_obj))
        kind = json_object_get_string(kind_obj);
    if (!kind) kind = "";

    char buf[256];
    int len = (int)strlen(spec);
    char conv = spec[len - 1];  /* last char is conversion specifier */

    if (strcmp(kind, "int") == 0 || strcmp(kind, "long") == 0 || strcmp(kind, "any") == 0 ||
        strcmp(kind, "uint") == 0)
    {
        /* 64-bit integer types: insert 'll' before conversion character */
        if (conv == 'd' || conv == 'i' || conv == 'x' || conv == 'X' ||
            conv == 'o' || conv == 'u')
        {
            snprintf(buf, sizeof(buf), "%%%.*sll%c", len - 1, spec, conv);
        }
        else
        {
            snprintf(buf, sizeof(buf), "%%%s", spec);
        }
    }
    else
    {
        /* double, float, string, char, byte, int32, uint32: use spec as-is */
        snprintf(buf, sizeof(buf), "%%%s", spec);
    }

    return strdup(buf);
}

static char *helper_box_func_min(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options; (void)param_count; (void)params;
    return strdup("/* box */");
}

static char *helper_unbox_func_min(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options; (void)param_count; (void)params;
    return strdup("/* unbox */");
}

/* ---- Register All Minimal C Helpers ---- */

static void register_min_c_helpers(hbs_env_t *env)
{
    hbs_register_helper(env, "eq", helper_eq);
    hbs_register_helper(env, "c_type", helper_c_type_min);
    hbs_register_helper(env, "default_value", helper_default_value_min);
    hbs_register_helper(env, "type_suffix", helper_type_suffix_min);
    hbs_register_helper(env, "c_sizeof", helper_c_sizeof_min);
    hbs_register_helper(env, "to_string_func", helper_to_string_func_min);
    hbs_register_helper(env, "op_symbol", helper_op_symbol);
    hbs_register_helper(env, "return_label", helper_return_label);
    hbs_register_helper(env, "type_tag", helper_type_tag_min);
    hbs_register_helper(env, "box_func", helper_box_func_min);
    hbs_register_helper(env, "unbox_func", helper_unbox_func_min);
    hbs_register_helper(env, "count", helper_count);
    hbs_register_helper(env, "printf_format", helper_printf_format);
}

/* ---- Public API ---- */

char *gen_model_render_min_c(json_object *model, const char *template_dir)
{
    return render_with_helpers(model, template_dir, register_min_c_helpers, "min_c");
}
