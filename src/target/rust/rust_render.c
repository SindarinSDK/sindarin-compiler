#include "target/rust/rust_render.h"
#include "cgen/gen_model_render_internal.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *json_kind(json_object *type)
{
    json_object *kind = NULL;
    if (!type || !json_object_object_get_ex(type, "kind", &kind)) return NULL;
    return json_object_get_string(kind);
}

static const char *json_string_property(json_object *object, const char *key)
{
    json_object *value = NULL;
    return object && json_object_object_get_ex(object, key, &value)
        ? json_object_get_string(value) : NULL;
}

static char *helper_rust_ident(json_object **params, int param_count, hbs_options_t *options);

static char *rust_type(json_object *type)
{
    const char *kind = json_kind(type);
    if (!kind) return strdup("()");
    if (strcmp(kind, "void") == 0) return strdup("()");
    if (strcmp(kind, "int") == 0 || strcmp(kind, "long") == 0) return strdup("i64");
    if (strcmp(kind, "int32") == 0) return strdup("i32");
    if (strcmp(kind, "uint") == 0) return strdup("u64");
    if (strcmp(kind, "uint32") == 0) return strdup("u32");
    if (strcmp(kind, "double") == 0) return strdup("f64");
    if (strcmp(kind, "float") == 0) return strdup("f32");
    if (strcmp(kind, "bool") == 0) return strdup("bool");
    if (strcmp(kind, "char") == 0) return strdup("char");
    if (strcmp(kind, "byte") == 0) return strdup("u8");
    if (strcmp(kind, "string") == 0) return strdup("String");
    if (strcmp(kind, "struct") == 0)
    {
        json_object *name_obj = NULL;
        if (!json_object_object_get_ex(type, "name", &name_obj)) return strdup("()");
        const char *name = json_object_get_string(name_obj);
        if (!name) return strdup("()");
        json_object *param = json_object_new_string(name);
        json_object *params[] = {param};
        char *result = helper_rust_ident(params, 1, NULL);
        json_object_put(param);
        return result;
    }
    return strdup("()");
}

static char *helper_rust_type(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    return param_count > 0 ? rust_type(params[0]) : strdup("()");
}

static bool rust_keyword(const char *name)
{
    static const char *keywords[] = {
        "as", "break", "const", "continue", "crate", "else", "enum", "extern",
        "false", "fn", "for", "if", "impl", "in", "let", "loop", "match",
        "mod", "move", "mut", "pub", "ref", "return", "self", "Self", "static",
        "struct", "super", "trait", "true", "type", "unsafe", "use", "where",
        "while", "async", "await", "dyn", "abstract", "become", "box", "do",
        "final", "macro", "override", "priv", "typeof", "unsized", "virtual",
        "yield", "try"
    };
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
        if (strcmp(name, keywords[i]) == 0) return true;
    return false;
}

static char *helper_rust_ident(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) return strdup("_");
    const char *name = json_object_get_string(params[0]);
    if (!name) return strdup("_");
    if (!rust_keyword(name)) return strdup(name);
    size_t length = strlen(name) + 3;
    char *escaped = malloc(length);
    if (!escaped) return NULL;
    snprintf(escaped, length, "r#%s", name);
    return escaped;
}

static char *quote_rust_string(const char *value, char quote)
{
    if (!value) value = "";
    size_t capacity = strlen(value) * 4 + 3;
    char *result = malloc(capacity);
    if (!result) return NULL;
    size_t out = 0;
    result[out++] = quote;
    for (const unsigned char *p = (const unsigned char *)value; *p; p++)
    {
        switch (*p)
        {
            case '\\': result[out++] = '\\'; result[out++] = '\\'; break;
            case '\n': result[out++] = '\\'; result[out++] = 'n'; break;
            case '\r': result[out++] = '\\'; result[out++] = 'r'; break;
            case '\t': result[out++] = '\\'; result[out++] = 't'; break;
            case '"':
                if (quote == '"') result[out++] = '\\';
                result[out++] = '"';
                break;
            case '\'':
                if (quote == '\'') result[out++] = '\\';
                result[out++] = '\'';
                break;
            default:
                if (*p < 0x20)
                    out += (size_t)snprintf(result + out, capacity - out, "\\u{%x}", *p);
                else
                    result[out++] = (char)*p;
        }
    }
    result[out++] = quote;
    result[out] = '\0';
    return result;
}

static char *helper_rust_literal(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) return strdup("()");
    json_object *node = params[0];
    json_object *kind_obj = NULL;
    json_object *value_obj = NULL;
    if (!json_object_object_get_ex(node, "value_kind", &kind_obj)) return strdup("()");
    const char *kind = json_object_get_string(kind_obj);
    json_object_object_get_ex(node, "value", &value_obj);

    if (strcmp(kind, "bool") == 0)
        return strdup(value_obj && json_object_get_boolean(value_obj) ? "true" : "false");
    if (strcmp(kind, "string") == 0)
    {
        char *quoted = quote_rust_string(value_obj ? json_object_get_string(value_obj) : "", '"');
        if (!quoted) return NULL;
        size_t length = strlen(quoted) + sizeof(".to_string()") + 1;
        char *result = malloc(length);
        if (result) snprintf(result, length, "%s.to_string()", quoted);
        free(quoted);
        return result;
    }
    if (strcmp(kind, "char") == 0)
        return quote_rust_string(value_obj ? json_object_get_string(value_obj) : "", '\'');
    if (strcmp(kind, "nil") == 0) return strdup("None");
    return strdup(value_obj ? json_object_get_string(value_obj) : "0");
}

static char *helper_rust_default(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    const char *kind = param_count > 0 ? json_kind(params[0]) : NULL;
    if (!kind) return strdup("()");
    if (strcmp(kind, "bool") == 0) return strdup("false");
    if (strcmp(kind, "char") == 0) return strdup("'\\0'");
    if (strcmp(kind, "string") == 0) return strdup("String::new()");
    if (strcmp(kind, "void") == 0) return strdup("()");
    if (strcmp(kind, "double") == 0 || strcmp(kind, "float") == 0) return strdup("0.0");
    return strdup("0");
}

static char *helper_rust_unary(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) return strdup("");
    const char *op = json_object_get_string(params[0]);
    if (!op) return strdup("");
    if (strcmp(op, "negate") == 0) return strdup("-");
    if (strcmp(op, "not") == 0) return strdup("!");
    if (strcmp(op, "bitnot") == 0) return strdup("!");
    return strdup("");
}

static char *helper_rust_clone_suffix(json_object **params, int param_count,
                                      hbs_options_t *options)
{
    (void)options;
    if (param_count < 2 || !params[0] || !params[1] ||
        !json_object_get_boolean(params[1])) return strdup("");

    const char *kind = json_string_property(params[0], "kind");
    if (kind && (strcmp(kind, "variable") == 0 || strcmp(kind, "member") == 0))
        return strdup(".clone()");
    return strdup("");
}

static char *helper_newline(json_object **params, int param_count, hbs_options_t *options)
{
    (void)params;
    (void)param_count;
    (void)options;
    return strdup("\n");
}

static char *helper_right_brace(json_object **params, int param_count, hbs_options_t *options)
{
    (void)params;
    (void)param_count;
    (void)options;
    return strdup("}");
}

static void register_rust_helpers(hbs_env_t *env)
{
    hbs_register_helper(env, "eq", helper_eq);
    hbs_register_helper(env, "op_symbol", helper_op_symbol);
    hbs_register_helper(env, "rust_type", helper_rust_type);
    hbs_register_helper(env, "rust_ident", helper_rust_ident);
    hbs_register_helper(env, "rust_literal", helper_rust_literal);
    hbs_register_helper(env, "rust_default", helper_rust_default);
    hbs_register_helper(env, "rust_unary", helper_rust_unary);
    hbs_register_helper(env, "rust_clone_suffix", helper_rust_clone_suffix);
    hbs_register_helper(env, "nl", helper_newline);
    hbs_register_helper(env, "rbrace", helper_right_brace);
}

char *rust_render_model(json_object *model, const char *template_dir)
{
    return render_with_helpers(model, template_dir, register_rust_helpers, "rust");
}
