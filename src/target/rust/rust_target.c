#include "target/target.h"
#include "target/rust/rust_render.h"
#include "cgen/gen_model.h"
#include "debug.h"
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *rustc_command(void)
{
    const char *configured = getenv("SN_RUSTC");
    return configured && configured[0] ? configured : "rustc";
}

static bool rustc_quoted(char *out, size_t out_size)
{
    const char *path = rustc_command();
    size_t len = strlen(path);
    size_t needed;
#ifdef _WIN32
    /* Windows: wrap the executable path in double quotes so it stays exactly
     * one token; a path ending in a backslash needs a doubled backslash before
     * the closing quote. */
    size_t trailing_bs = (len > 0 && path[len - 1] == '\\') ? 1 : 0;
    needed = 2 + len + trailing_bs;
    if (needed >= out_size) return false;
    char *wcursor = out;
    size_t wremaining = out_size;
    *wcursor++ = '"';
    wremaining--;
    for (size_t i = 0; i < len; i++)
    {
        *wcursor++ = path[i];
        wremaining--;
    }
    if (trailing_bs)
    {
        *wcursor++ = '\\';
        wremaining--;
    }
    *wcursor++ = '"';
    wremaining--;
    *wcursor = '\0';
    (void)wremaining;
#else
    /* POSIX: shell-quote the path as one token so spaces and ampersands stay
     * literal. An embedded single quote is encoded with the standard
     * close-quote / escaped-quote / reopen-quote sequence. */
    size_t apostrophes = 0;
    for (size_t i = 0; i < len; i++)
        if (path[i] == '\'')
            apostrophes++;
    needed = 2 + len + 3 * apostrophes;
    if (needed >= out_size) return false;

    char *cursor = out;
    size_t remaining = out_size;
    *cursor++ = '\'';
    remaining--;
    for (size_t i = 0; i < len; i++)
    {
        if (path[i] == '\'')
        {
            static const char seq[4] = { '\'', '\\', '\'', '\'' };
            memcpy(cursor, seq, sizeof(seq));
            cursor += sizeof(seq);
            remaining -= sizeof(seq);
        }
        else
        {
            *cursor++ = path[i];
            remaining--;
        }
    }
    *cursor++ = '\'';
    remaining--;
    *cursor = '\0';
    (void)remaining;
    #endif
    return true;
}

static bool rust_run_command(const char *command)
{
#ifdef _WIN32
    /* Windows cmd.exe /c strips the first opening quote from a command that
     * begins with a quoted executable path (so a spaced SN_RUSTC path splits
     * at the first space). Wrap the complete command in one additional outer
     * pair of double quotes so the executable path stays a single token. */
    size_t len = strlen(command);
    char *to_run = (char *)malloc(len + 3);
    if (!to_run)
    {
        fprintf(stderr, "Error: failed to allocate buffer for rustc invocation\n");
        return false;
    }
    to_run[0] = '"';
    memcpy(to_run + 1, command, len);
    to_run[len + 1] = '"';
    to_run[len + 2] = '\0';
    int status = system(to_run);
    free(to_run);
    return status == 0;
#else
    /* POSIX: pass the command through unchanged; no extra allocation. */
    return system(command) == 0;
#endif
}

static bool rust_check_toolchain(const CompilerOptions *options)
{
    char command[PATH_MAX + 64];
    char quoted_rustc[PATH_MAX + 8];
    if (!rustc_quoted(quoted_rustc, sizeof(quoted_rustc)))
    {
        fprintf(stderr,
                "Error: SN_RUSTC path is too long to shell-quote safely; use --emit-rust.\n");
        return false;
    }
    int written;
#ifdef _WIN32
    written = snprintf(command, sizeof(command), "%s --version >NUL 2>&1", quoted_rustc);
#else
    written = snprintf(command, sizeof(command), "%s --version >/dev/null 2>&1", quoted_rustc);
#endif
    if (written < 0 || (size_t)written >= sizeof(command))
    {
        fprintf(stderr, "Error: failed to build rustc --version command\n");
        return false;
    }
    if (rust_run_command(command))
    {
        if (options->verbose) DEBUG_INFO("Rust compiler '%s' found", rustc_command());
        return true;
    }
    fprintf(stderr, "Error: Rust compiler '%s' is not installed or not in PATH.\n", rustc_command());
    fprintf(stderr, "Set SN_RUSTC to a different compiler, or use --emit-rust.\n");
    return false;
}

static bool array_is_empty(json_object *object, const char *key)
{
    json_object *array = NULL;
    return !json_object_object_get_ex(object, key, &array) ||
           json_object_array_length(array) == 0;
}

static bool rust_type_supported(json_object *type)
{
    json_object *kind_obj = NULL;
    if (!type || !json_object_object_get_ex(type, "kind", &kind_obj)) return false;
    const char *kind = json_object_get_string(kind_obj);
    if (!kind) return false;
    if (strcmp(kind, "array") == 0)
    {
        json_object *element_type = NULL;
        return json_object_object_get_ex(type, "element_type", &element_type) &&
               rust_type_supported(element_type);
    }
    return strcmp(kind, "void") == 0 || strcmp(kind, "int") == 0 ||
        strcmp(kind, "long") == 0 || strcmp(kind, "int32") == 0 ||
        strcmp(kind, "uint") == 0 || strcmp(kind, "uint32") == 0 ||
        strcmp(kind, "double") == 0 || strcmp(kind, "float") == 0 ||
        strcmp(kind, "bool") == 0 || strcmp(kind, "char") == 0 ||
        strcmp(kind, "byte") == 0 || strcmp(kind, "string") == 0 ||
        strcmp(kind, "struct") == 0;
}

static const char *json_string_property(json_object *object, const char *key)
{
    json_object *value = NULL;
    return object && json_object_object_get_ex(object, key, &value)
        ? json_object_get_string(value) : NULL;
}

static bool json_boolean_property(json_object *object, const char *key)
{
    json_object *value = NULL;
    return object && json_object_object_get_ex(object, key, &value) &&
           json_object_get_boolean(value);
}

static bool json_string_property_equals(json_object *object, const char *key,
                                        const char *wanted)
{
    const char *value = json_string_property(object, key);
    return value && strcmp(value, wanted) == 0;
}

static bool rust_typeof_type_supported(json_object *type)
{
    const char *kind = json_string_property(type, "kind");
    if (!kind) return false;
    if (strcmp(kind, "array") == 0)
    {
        json_object *element_type = NULL;
        return json_object_object_get_ex(type, "element_type", &element_type) &&
               rust_typeof_type_supported(element_type);
    }
    if (strcmp(kind, "struct") == 0)
        return !json_boolean_property(type, "is_native") &&
               !json_boolean_property(type, "is_packed") &&
               !json_boolean_property(type, "pass_self_by_ref");
    return strcmp(kind, "int") == 0 || strcmp(kind, "long") == 0 ||
        strcmp(kind, "int32") == 0 || strcmp(kind, "uint") == 0 ||
        strcmp(kind, "uint32") == 0 || strcmp(kind, "double") == 0 ||
        strcmp(kind, "float") == 0 || strcmp(kind, "bool") == 0 ||
        strcmp(kind, "char") == 0 || strcmp(kind, "byte") == 0 ||
        strcmp(kind, "string") == 0;
}

static bool rust_validate_typeof_operand(json_object *expr,
                                         json_object *reflected_type)
{
    const char *kind = json_string_property(reflected_type, "kind");
    if (json_boolean_property(expr, "reflected_is_sized_array"))
    {
        fprintf(stderr,
                "Error: Rust target does not support typeOf for sized-array operands\n");
        return false;
    }
    if (kind && strcmp(kind, "void") == 0)
    {
        fprintf(stderr,
                "Error: Rust target does not support typeOf for void operands\n");
        return false;
    }
    if (!rust_typeof_type_supported(reflected_type))
    {
        fprintf(stderr,
                "Error: Rust target does not support typeOf for this operand type yet\n");
        return false;
    }
    return true;
}

static bool rust_scalar_ref_parameter_type_supported(json_object *type)
{
    return json_string_property_equals(type, "kind", "int") ||
        json_string_property_equals(type, "kind", "long") ||
        json_string_property_equals(type, "kind", "int32") ||
        json_string_property_equals(type, "kind", "byte") ||
        json_string_property_equals(type, "kind", "uint32") ||
        json_string_property_equals(type, "kind", "uint") ||
        json_string_property_equals(type, "kind", "bool") ||
        json_string_property_equals(type, "kind", "float") ||
        json_string_property_equals(type, "kind", "double");
}

static bool rust_floating_type(json_object *type)
{
    return json_string_property_equals(type, "kind", "float") ||
        json_string_property_equals(type, "kind", "double");
}

static bool rust_checked_scalar_ref_parameter(json_object *mutation,
                                               json_object *parameter)
{
    json_object *type = NULL;
    return json_string_property_equals(mutation, "mutation_storage", "parameter") &&
        json_string_property_equals(mutation, "mutation_place", "variable") &&
        json_string_property_equals(parameter, "kind", "variable") &&
        json_string_property_equals(parameter, "parameter_mem_qual", "as_ref") &&
        json_object_object_get_ex(parameter, "type", &type) &&
        (json_string_property_equals(type, "kind", "int") ||
         json_string_property_equals(type, "kind", "long") ||
         json_string_property_equals(type, "kind", "int32") ||
         json_string_property_equals(type, "kind", "byte") ||
         json_string_property_equals(type, "kind", "uint32") ||
         json_string_property_equals(type, "kind", "uint"));
}

static bool rust_validate_structs(json_object *model)
{
    json_object *structs = NULL;
    if (!json_object_object_get_ex(model, "structs", &structs)) return true;

    size_t count = json_object_array_length(structs);
    for (size_t i = 0; i < count; i++)
    {
        json_object *structure = json_object_array_get_idx(structs, i);
        const char *name = json_string_property(structure, "name");
        const char *mem_mode = json_string_property(structure, "mem_mode");
        json_object *fields = NULL;

        if (name && (strcmp(name, "FieldInfo") == 0 ||
                     strcmp(name, "TypeInfo") == 0))
        {
            fprintf(stderr,
                    "Error: Rust target reserves struct name '%s' for compiler reflection metadata\n",
                    name);
            return false;
        }

        if (json_boolean_property(structure, "is_native") ||
            json_boolean_property(structure, "is_packed") ||
            json_boolean_property(structure, "is_serializable") ||
            (mem_mode && strcmp(mem_mode, "val") != 0))
        {
            fprintf(stderr,
                    "Error: Rust target currently supports only plain value struct '%s'\n",
                    name ? name : "<anonymous>");
            return false;
        }

        if (!json_object_object_get_ex(structure, "fields", &fields)) continue;
        size_t field_count = json_object_array_length(fields);
        for (size_t f = 0; f < field_count; f++)
        {
            json_object *field = json_object_array_get_idx(fields, f);
            json_object *type = NULL;
            const char *field_name = json_string_property(field, "name");
            if (!json_object_object_get_ex(field, "type", &type) ||
                !rust_type_supported(type))
            {
                fprintf(stderr,
                        "Error: Rust target does not support field '%s.%s' yet\n",
                        name ? name : "<anonymous>",
                        field_name ? field_name : "<anonymous>");
                return false;
            }
        }
    }
    return true;
}

static json_object *rust_validation_model;
static bool rust_validation_reported_error;

static bool rust_validate_expr(json_object *expr);
static bool rust_validate_value_match(json_object *expr);

static bool rust_report_match_error(const char *message)
{
    rust_validation_reported_error = true;
    fprintf(stderr, "Error: Rust target %s\n", message);
    return false;
}

static bool rust_array_method_supported(const char *name)
{
    if (!name) return false;
    return strcmp(name, "push") == 0 || strcmp(name, "pop") == 0 ||
           strcmp(name, "insert") == 0 || strcmp(name, "remove") == 0 ||
           strcmp(name, "reverse") == 0 || strcmp(name, "clear") == 0 ||
           strcmp(name, "clone") == 0 || strcmp(name, "contains") == 0 ||
           strcmp(name, "indexOf") == 0 || strcmp(name, "concat") == 0 ||
           strcmp(name, "join") == 0;
}

static bool rust_string_method_supported(const char *name)
{
    if (!name) return false;
    return strcmp(name, "contains") == 0 || strcmp(name, "startsWith") == 0 ||
           strcmp(name, "endsWith") == 0 || strcmp(name, "trim") == 0 ||
           strcmp(name, "toUpper") == 0 || strcmp(name, "toLower") == 0 ||
           strcmp(name, "substring") == 0 || strcmp(name, "replace") == 0 ||
           strcmp(name, "charAt") == 0 || strcmp(name, "indexOf") == 0;
}

/* Keep this list aligned with the source-language primitive conversion members.
 * Unsupported source-valid conversions receive a target-specific diagnostic below;
 * model-only names such as toFloat/toBool deliberately are not recognized here. */
static bool rust_primitive_conversion_member(const char *type_kind, const char *name)
{
    if (!type_kind || !name) return false;
    if (strcmp(type_kind, "int") == 0)
        return strcmp(name, "toDouble") == 0 || strcmp(name, "toLong") == 0 ||
               strcmp(name, "toUint") == 0 || strcmp(name, "toByte") == 0 ||
               strcmp(name, "toChar") == 0;
    if (strcmp(type_kind, "long") == 0)
        return strcmp(name, "toInt") == 0 || strcmp(name, "toDouble") == 0;
    if (strcmp(type_kind, "double") == 0)
        return strcmp(name, "toInt") == 0 || strcmp(name, "toLong") == 0;
    if (strcmp(type_kind, "uint") == 0)
        return strcmp(name, "toInt") == 0 || strcmp(name, "toLong") == 0 ||
               strcmp(name, "toDouble") == 0;
    if (strcmp(type_kind, "byte") == 0)
        return strcmp(name, "toInt") == 0 || strcmp(name, "toChar") == 0;
    if (strcmp(type_kind, "bool") == 0)
        return strcmp(name, "toInt") == 0;
    if (strcmp(type_kind, "char") == 0)
        return strcmp(name, "toInt") == 0;
    if (strcmp(type_kind, "string") == 0)
        return strcmp(name, "toInt") == 0 || strcmp(name, "toLong") == 0 ||
               strcmp(name, "toDouble") == 0;
    return false;
}

static bool rust_primitive_integer_conversion_supported(const char *type_kind,
                                                        const char *name)
{
    return (strcmp(type_kind, "int") == 0 &&
            (strcmp(name, "toDouble") == 0 || strcmp(name, "toLong") == 0 ||
             strcmp(name, "toUint") == 0 ||
             strcmp(name, "toByte") == 0)) ||
           (strcmp(type_kind, "long") == 0 &&
            (strcmp(name, "toInt") == 0 || strcmp(name, "toDouble") == 0)) ||
           (strcmp(type_kind, "uint") == 0 && strcmp(name, "toDouble") == 0) ||
           (strcmp(type_kind, "byte") == 0 && strcmp(name, "toInt") == 0) ||
           (strcmp(type_kind, "bool") == 0 && strcmp(name, "toInt") == 0);
}

typedef struct
{
    bool left_align;
    bool force_sign;
    bool space_sign;
    bool alternate;
    bool zero_pad;
    bool has_width;
    bool has_precision;
    int width;
    int precision;
    char conversion;
    char rust_format[64];
} RustFormatSpec;

static bool rust_integer_type(const char *kind)
{
    return kind && (strcmp(kind, "int") == 0 || strcmp(kind, "long") == 0 ||
                    strcmp(kind, "int32") == 0 || strcmp(kind, "uint") == 0 ||
                    strcmp(kind, "uint32") == 0 || strcmp(kind, "byte") == 0);
}

static bool rust_float_type(const char *kind)
{
    return kind && (strcmp(kind, "double") == 0 || strcmp(kind, "float") == 0);
}

static int rust_fixed_sizeof_bytes(const char *kind)
{
    if (!kind) return -1;
    if (strcmp(kind, "int") == 0 || strcmp(kind, "long") == 0 ||
        strcmp(kind, "uint") == 0 || strcmp(kind, "double") == 0 ||
        strcmp(kind, "string") == 0 || strcmp(kind, "array") == 0)
        return 8;
    if (strcmp(kind, "int32") == 0 || strcmp(kind, "uint32") == 0 ||
        strcmp(kind, "float") == 0)
        return 4;
    if (strcmp(kind, "byte") == 0 || strcmp(kind, "bool") == 0 ||
        strcmp(kind, "char") == 0)
        return 1;
    return -1;
}

static void rust_report_unsupported_sizeof(json_object *type)
{
    const char *kind = json_string_property(type, "kind");
    const char *name = json_string_property(type, "name");
    const char *category = "type";

    if (kind && strcmp(kind, "string") == 0)
        category = "dynamic type";
    else if (kind && (strcmp(kind, "array") == 0 ||
                      strcmp(kind, "struct") == 0))
        category = "aggregate type";
    else if (kind && strcmp(kind, "pointer") == 0)
        category = "pointer type";
    else if (kind && (strcmp(kind, "void") == 0 || strcmp(kind, "nil") == 0))
        category = "non-value type";

    if (kind && strcmp(kind, "struct") == 0 && name)
        fprintf(stderr,
                "Error: Rust target does not support sizeof for aggregate struct type '%s'; only fixed-size scalar or managed-handle types are supported\n",
                name);
    else
        fprintf(stderr,
                "Error: Rust target does not support sizeof for %s '%s'; only fixed-size scalar or managed-handle types are supported\n",
                category, kind ? kind : "<unknown>");
}

static bool rust_signed_integer_type(const char *kind)
{
    return kind && (strcmp(kind, "int") == 0 || strcmp(kind, "long") == 0 ||
                    strcmp(kind, "int32") == 0);
}

static bool rust_unsigned_integer_type(const char *kind)
{
    return kind && (strcmp(kind, "uint") == 0 || strcmp(kind, "uint32") == 0 ||
                    strcmp(kind, "byte") == 0);
}

static bool rust_array_search_type_supported(const char *kind)
{
    return rust_integer_type(kind) ||
           (kind && (strcmp(kind, "float") == 0 || strcmp(kind, "double") == 0)) ||
           (kind && (strcmp(kind, "bool") == 0 || strcmp(kind, "char") == 0 ||
                     strcmp(kind, "string") == 0));
}

static json_object *rust_find_struct(json_object *model, const char *name)
{
    json_object *structs = NULL;
    if (!model || !name || !json_object_object_get_ex(model, "structs", &structs))
        return NULL;

    size_t count = json_object_array_length(structs);
    for (size_t i = 0; i < count; i++)
    {
        json_object *structure = json_object_array_get_idx(structs, i);
        if (json_string_property_equals(structure, "name", name)) return structure;
    }
    return NULL;
}

static const char *rust_reachable_user_copy_struct(json_object *type,
                                                   json_object *visiting)
{
    const char *kind = json_string_property(type, "kind");
    if (!kind) return NULL;

    if (strcmp(kind, "array") == 0)
    {
        json_object *element_type = NULL;
        return json_object_object_get_ex(type, "element_type", &element_type)
            ? rust_reachable_user_copy_struct(element_type, visiting) : NULL;
    }
    if (strcmp(kind, "struct") != 0) return NULL;

    const char *name = json_string_property(type, "name");
    json_object *structure = rust_find_struct(rust_validation_model, name);
    if (!structure) return NULL;
    if (json_boolean_property(structure, "has_user_copy_method")) return name;

    json_object *seen = NULL;
    if (!name || json_object_object_get_ex(visiting, name, &seen)) return NULL;
    json_object_object_add(visiting, name, json_object_new_boolean(true));

    json_object *fields = NULL;
    if (json_object_object_get_ex(structure, "fields", &fields))
    {
        size_t field_count = json_object_array_length(fields);
        for (size_t i = 0; i < field_count; i++)
        {
            json_object *field = json_object_array_get_idx(fields, i);
            json_object *field_type = NULL;
            if (json_object_object_get_ex(field, "type", &field_type))
            {
                const char *user_copy =
                    rust_reachable_user_copy_struct(field_type, visiting);
                if (user_copy)
                {
                    json_object_object_del(visiting, name);
                    return user_copy;
                }
            }
        }
    }

    json_object_object_del(visiting, name);
    return NULL;
}

static bool rust_auto_copy_plain_value_struct_type(
    json_object *type, const char **user_copy_name)
{
    if (user_copy_name) *user_copy_name = NULL;
    if (!json_string_property_equals(type, "kind", "struct")) return false;

    json_object *structure = rust_find_struct(
        rust_validation_model, json_string_property(type, "name"));
    if (!structure) return false;

    /* rust_validate_structs runs first and validates every declared field. */
    const char *mem_mode = json_string_property(structure, "mem_mode");
    if (!mem_mode || strcmp(mem_mode, "val") != 0 ||
        json_boolean_property(structure, "is_native") ||
        json_boolean_property(structure, "is_packed") ||
        json_boolean_property(structure, "is_serializable")) return false;

    json_object *visiting = json_object_new_object();
    if (!visiting) return false;
    const char *user_copy = rust_reachable_user_copy_struct(type, visiting);
    json_object_put(visiting);
    if (user_copy_name) *user_copy_name = user_copy;
    return !user_copy;
}

static bool rust_array_concat_type_supported(json_object *type)
{
    const char *kind = json_string_property(type, "kind");
    if (kind && strcmp(kind, "struct") == 0)
    {
        json_object *structure = rust_find_struct(
            rust_validation_model, json_string_property(type, "name"));
        return structure && !json_boolean_property(structure, "has_heap_fields");
    }
    return rust_integer_type(kind) || rust_float_type(kind) ||
           (kind && (strcmp(kind, "bool") == 0 || strcmp(kind, "char") == 0 ||
                     strcmp(kind, "string") == 0));
}

static bool rust_heap_free_named_struct_type(json_object *type)
{
    if (!json_string_property_equals(type, "kind", "struct")) return false;
    json_object *structure = rust_find_struct(
        rust_validation_model, json_string_property(type, "name"));
    return structure && !json_boolean_property(structure, "has_heap_fields");
}

static bool rust_array_copy_type_supported(json_object *type)
{
    const char *kind = json_string_property(type, "kind");
    if (kind && strcmp(kind, "struct") == 0)
        return rust_heap_free_named_struct_type(type);
    return rust_integer_type(kind) || rust_float_type(kind) ||
           (kind && (strcmp(kind, "bool") == 0 || strcmp(kind, "char") == 0 ||
                     strcmp(kind, "string") == 0));
}

static bool rust_parse_format_spec(const char *spec, const char *type_kind,
                                   RustFormatSpec *parsed, char *reason,
                                   size_t reason_size)
{
    memset(parsed, 0, sizeof(*parsed));
    if (!spec || !spec[0])
    {
        snprintf(reason, reason_size, "empty format specifier");
        return false;
    }

    const char *cursor = spec;
    while (*cursor)
    {
        switch (*cursor)
        {
            case '-': parsed->left_align = true; break;
            case '+': parsed->force_sign = true; break;
            case '#': parsed->alternate = true; break;
            case '0': parsed->zero_pad = true; break;
            case ' ': parsed->space_sign = true; break;
            default: goto flags_done;
        }
        cursor++;
    }
flags_done:

    if (isdigit((unsigned char)*cursor))
    {
        parsed->has_width = true;
        while (isdigit((unsigned char)*cursor))
        {
            int digit = *cursor++ - '0';
            if (parsed->width > (1000 - digit) / 10)
            {
                snprintf(reason, reason_size, "format width is too large");
                return false;
            }
            parsed->width = parsed->width * 10 + digit;
        }
    }
    if (*cursor == '.')
    {
        cursor++;
        parsed->has_precision = true;
        if (!isdigit((unsigned char)*cursor))
        {
            snprintf(reason, reason_size, "format precision requires digits");
            return false;
        }
        while (isdigit((unsigned char)*cursor))
        {
            int digit = *cursor++ - '0';
            if (parsed->precision > (1000 - digit) / 10)
            {
                snprintf(reason, reason_size, "format precision is too large");
                return false;
            }
            parsed->precision = parsed->precision * 10 + digit;
        }
    }
    if (!*cursor || cursor[1])
    {
        snprintf(reason, reason_size, "invalid conversion suffix");
        return false;
    }
    parsed->conversion = *cursor;

    bool is_integer_conversion = strchr("diuxXo", parsed->conversion) != NULL;
    bool is_fixed_conversion = parsed->conversion == 'f';
    bool is_scientific_conversion = parsed->conversion == 'e' ||
                                    parsed->conversion == 'E';
    bool is_float_conversion = is_fixed_conversion || is_scientific_conversion;
    bool is_string_conversion = parsed->conversion == 's';
    bool is_character_conversion = parsed->conversion == 'c';
    if (!is_integer_conversion && !is_float_conversion &&
        !is_string_conversion && !is_character_conversion)
    {
        snprintf(reason, reason_size, "unsupported conversion '%c'", parsed->conversion);
        return false;
    }
    if (is_integer_conversion && !rust_integer_type(type_kind))
    {
        snprintf(reason, reason_size, "integer conversion requires an integer expression");
        return false;
    }
    if ((parsed->conversion == 'd' || parsed->conversion == 'i') &&
        !rust_signed_integer_type(type_kind))
    {
        snprintf(reason, reason_size, "signed decimal conversion requires a signed integer");
        return false;
    }
    if (parsed->conversion == 'u' && !rust_unsigned_integer_type(type_kind))
    {
        snprintf(reason, reason_size, "unsigned decimal conversion requires an unsigned integer");
        return false;
    }
    if (is_float_conversion && !rust_float_type(type_kind))
    {
        snprintf(reason, reason_size, "floating-point conversion requires a float expression");
        return false;
    }
    if (is_string_conversion && (!type_kind || strcmp(type_kind, "string") != 0))
    {
        snprintf(reason, reason_size, "string conversion requires a string expression");
        return false;
    }
    if (is_character_conversion &&
        (!type_kind || strcmp(type_kind, "char") != 0))
    {
        snprintf(reason, reason_size,
                 "character conversion requires a char expression");
        return false;
    }
    if (parsed->has_precision && !is_float_conversion && !is_string_conversion)
    {
        snprintf(reason, reason_size,
                 "precision is supported only for floating-point and string conversions");
        return false;
    }
    if (is_string_conversion &&
        (parsed->force_sign || parsed->space_sign || parsed->alternate ||
         parsed->zero_pad))
    {
        snprintf(reason, reason_size, "numeric flags cannot format strings");
        return false;
    }
    if (is_character_conversion &&
        (parsed->force_sign || parsed->space_sign || parsed->alternate ||
         parsed->zero_pad))
    {
        snprintf(reason, reason_size,
                 "numeric flags cannot format characters");
        return false;
    }
    if (is_character_conversion) return true;
    if (parsed->alternate && parsed->conversion != 'x' &&
        parsed->conversion != 'X' && parsed->conversion != 'o' &&
        parsed->conversion != 'f' && parsed->conversion != 'e' &&
        parsed->conversion != 'E')
    {
        snprintf(reason, reason_size,
                 "alternate form is supported only for hexadecimal, octal, fixed-point, and scientific conversions");
        return false;
    }
    if ((parsed->conversion == 'u' || parsed->conversion == 'x' ||
         parsed->conversion == 'X' || parsed->conversion == 'o') && parsed->force_sign)
    {
        snprintf(reason, reason_size, "sign flag is not valid for this conversion");
        return false;
    }
    if (is_scientific_conversion) return true;

    char *out = parsed->rust_format;
    size_t remaining = sizeof(parsed->rust_format);
    int written = snprintf(out, remaining, "{:");
    out += written;
    remaining -= (size_t)written;

    if (parsed->left_align)
    {
        written = snprintf(out, remaining, "<");
        out += written;
        remaining -= (size_t)written;
    }
    else if (is_string_conversion && parsed->has_width)
    {
        written = snprintf(out, remaining, ">");
        out += written;
        remaining -= (size_t)written;
    }
    if (parsed->force_sign ||
        (parsed->space_sign &&
         ((parsed->conversion == 'd' || parsed->conversion == 'i') ||
          is_fixed_conversion)))
    {
        written = snprintf(out, remaining, "+");
        out += written;
        remaining -= (size_t)written;
    }
    if (parsed->zero_pad && !parsed->left_align)
    {
        written = snprintf(out, remaining, "0");
        out += written;
        remaining -= (size_t)written;
    }
    if (parsed->has_width)
    {
        written = snprintf(out, remaining, "%d", parsed->width);
        out += written;
        remaining -= (size_t)written;
    }
    if (is_fixed_conversion)
    {
        int precision = parsed->has_precision ? parsed->precision : 6;
        written = snprintf(out, remaining, ".%d", precision);
        out += written;
        remaining -= (size_t)written;
    }
    if (parsed->conversion == 'x' || parsed->conversion == 'X' ||
        parsed->conversion == 'o')
    {
        written = snprintf(out, remaining, "%c", parsed->conversion);
        out += written;
        remaining -= (size_t)written;
    }
    snprintf(out, remaining, "}");
    return true;
}

static bool rust_validate_expr_array(json_object *array)
{
    if (!array) return true;
    size_t count = json_object_array_length(array);
    for (size_t i = 0; i < count; i++)
        if (!rust_validate_expr(json_object_array_get_idx(array, i))) return false;
    return true;
}

/* These checks deliberately mirror parser_init.c's compiler-injected
 * FieldInfo/TypeInfo topology.  Rust emits native equivalents, so fail closed
 * if the shared front-end definition changes without this backend changing in
 * lockstep. */
static bool rust_reflection_field_is(json_object *field, const char *name,
                                     const char *kind)
{
    json_object *type = NULL;
    return field && json_string_property_equals(field, "name", name) &&
        json_object_object_get_ex(field, "type", &type) &&
        json_string_property_equals(type, "kind", kind);
}

static bool rust_reflection_schema_is_current(json_object *type_info)
{
    json_object *fields = NULL;
    if (!json_string_property_equals(type_info, "kind", "struct") ||
        !json_string_property_equals(type_info, "name", "TypeInfo") ||
        !json_object_object_get_ex(type_info, "fields", &fields) ||
        json_object_array_length(fields) != 4 ||
        !rust_reflection_field_is(json_object_array_get_idx(fields, 0), "name", "string") ||
        !rust_reflection_field_is(json_object_array_get_idx(fields, 1), "fields", "array") ||
        !rust_reflection_field_is(json_object_array_get_idx(fields, 2), "fieldCount", "int") ||
        !rust_reflection_field_is(json_object_array_get_idx(fields, 3), "typeId", "int"))
        return false;

    json_object *fields_type = NULL, *field_info = NULL, *field_info_fields = NULL;
    json_object *fields_field = json_object_array_get_idx(fields, 1);
    return json_object_object_get_ex(fields_field, "type", &fields_type) &&
        json_object_object_get_ex(fields_type, "element_type", &field_info) &&
        json_string_property_equals(field_info, "kind", "struct") &&
        json_string_property_equals(field_info, "name", "FieldInfo") &&
        json_object_object_get_ex(field_info, "fields", &field_info_fields) &&
        json_object_array_length(field_info_fields) == 3 &&
        rust_reflection_field_is(json_object_array_get_idx(field_info_fields, 0), "name", "string") &&
        rust_reflection_field_is(json_object_array_get_idx(field_info_fields, 1), "typeName", "string") &&
        rust_reflection_field_is(json_object_array_get_idx(field_info_fields, 2), "typeId", "int");
}

static bool rust_validate_expr(json_object *expr)
{
    json_object *kind_obj = NULL;
    if (!expr || !json_object_object_get_ex(expr, "kind", &kind_obj)) return false;
    const char *kind = json_object_get_string(kind_obj);
    json_object *child = NULL;
    if (!kind) return false;

    if (strcmp(kind, "match") == 0)
        return rust_validate_value_match(expr);

    if (strcmp(kind, "literal") == 0)
    {
        json_object *reflected_type = NULL;
        if (json_object_object_get_ex(expr, "reflected_type", &reflected_type))
            return rust_validate_typeof_operand(expr, reflected_type);
        return true;
    }
    if (strcmp(kind, "variable") == 0) return true;
    if (strcmp(kind, "typeof") == 0)
    {
        json_object *type_info = NULL, *reflected_type = NULL;
        if (!json_object_object_get_ex(expr, "type", &type_info) ||
            !rust_reflection_schema_is_current(type_info))
        {
            fprintf(stderr,
                    "Error: Rust target reflection metadata is out of sync with the built-in TypeInfo/FieldInfo definitions\n");
            return false;
        }
        if (!json_object_object_get_ex(expr, "reflected_type", &reflected_type))
        {
            fprintf(stderr,
                    "Error: Rust target does not support typeOf for this operand type yet\n");
            return false;
        }
        if (!rust_validate_typeof_operand(expr, reflected_type)) return false;
        /* typeOf is compile-time and non-evaluating.  Its operand is omitted
         * from the shared expression model; validate only the resolved type. */
        return true;
    }
    if (strcmp(kind, "sizeof") == 0)
    {
        json_object *target_type = NULL;
        const char *target_kind = NULL;
        if (!json_object_object_get_ex(expr, "target_type", &target_type) ||
            !(target_kind = json_string_property(target_type, "kind")))
        {
            fprintf(stderr,
                    "Error: Rust target encountered sizeof without a resolved operand type\n");
            return false;
        }

        int bytes = rust_fixed_sizeof_bytes(target_kind);
        if (bytes < 0)
        {
            rust_report_unsupported_sizeof(target_type);
            return false;
        }

        /* sizeof is compile-time and non-evaluating. Keep the modeled operand
         * opaque: validation and rendering consume only its resolved type. */
        json_object_object_add(expr, "rust_sizeof_bytes", json_object_new_int(bytes));
        return true;
    }
    if (strcmp(kind, "struct_literal") == 0)
    {
        json_object *fields = NULL;
        if (!json_object_object_get_ex(expr, "fields", &fields)) return true;
        size_t count = json_object_array_length(fields);
        for (size_t i = 0; i < count; i++)
        {
            json_object *field = json_object_array_get_idx(fields, i);
            if (!json_object_object_get_ex(field, "value", &child) ||
                !rust_validate_expr(child)) return false;
        }
        return true;
    }
    if (strcmp(kind, "array_literal") == 0)
    {
        json_object *elements = NULL;
        if (!json_object_object_get_ex(expr, "elements", &elements)) return true;
        size_t count = json_object_array_length(elements);
        bool needs_flattening = false;
        for (size_t i = 0; i < count; i++)
        {
            json_object *element = json_object_array_get_idx(elements, i);
            const char *element_kind = json_string_property(element, "kind");
            if (!element_kind) return false;
            if (strcmp(element_kind, "spread") == 0)
            {
                json_object *operand = NULL;
                if (!json_object_object_get_ex(element, "operand", &operand) ||
                    !rust_validate_expr(operand)) return false;
                needs_flattening = true;
            }
            else
            {
                if (!rust_validate_expr(element)) return false;
                if (strcmp(element_kind, "range") == 0) needs_flattening = true;
            }
        }
        if (needs_flattening)
            json_object_object_add(expr, "rust_flatten",
                                   json_object_new_boolean(true));
        return true;
    }
    if (strcmp(kind, "range") == 0)
    {
        json_object *start = NULL, *end = NULL;
        return json_object_object_get_ex(expr, "start", &start) &&
               json_object_object_get_ex(expr, "end", &end) &&
               rust_validate_expr(start) && rust_validate_expr(end);
    }
    if (strcmp(kind, "interpolated_string") == 0)
    {
        json_object *parts = NULL;
        if (!json_object_object_get_ex(expr, "parts", &parts)) return true;
        size_t count = json_object_array_length(parts);
        for (size_t i = 0; i < count; i++)
        {
            json_object *part = json_object_array_get_idx(parts, i);
            const char *part_kind = json_string_property(part, "kind");
            json_object *value = NULL, *format_spec = NULL;
            if (!part_kind) return false;
            if (strcmp(part_kind, "text") == 0) continue;
            if (strcmp(part_kind, "expr") != 0 ||
                !json_object_object_get_ex(part, "expr", &value) ||
                !rust_validate_expr(value)) return false;
            if (json_object_object_get_ex(part, "format_spec", &format_spec))
            {
                json_object *type = NULL;
                RustFormatSpec parsed;
                char reason[160];
                const char *spec = json_object_get_string(format_spec);
                const char *type_kind = NULL;
                reason[0] = '\0';
                if (!json_object_object_get_ex(value, "type", &type) ||
                    !(type_kind = json_string_property(type, "kind")) ||
                    !rust_parse_format_spec(spec, type_kind, &parsed,
                                            reason, sizeof(reason)))
                {
                    fprintf(stderr,
                            "Error: Rust target does not support interpolation format '%s' for %s: %s\n",
                            spec ? spec : "", type_kind ? type_kind : "<unknown>",
                            reason[0] ? reason : "missing expression type");
                    return false;
                }
            }
        }
        return true;
    }
    if (strcmp(kind, "sized_array") == 0)
    {
        json_object *element_type = NULL, *size = NULL;
        if (!json_object_object_get_ex(expr, "element_type", &element_type) ||
            !json_object_object_get_ex(expr, "size", &size) ||
            !rust_type_supported(element_type)) return false;
        const char *element_kind = json_string_property(element_type, "kind");
        return element_kind && strcmp(element_kind, "struct") != 0 &&
               rust_validate_expr(size);
    }
    if (strcmp(kind, "array_access") == 0)
    {
        json_object *array = NULL, *index = NULL;
        return json_object_object_get_ex(expr, "array", &array) &&
               json_object_object_get_ex(expr, "index", &index) &&
               rust_validate_expr(array) && rust_validate_expr(index);
    }
    if (strcmp(kind, "array_slice") == 0)
    {
        json_object *array = NULL, *start = NULL, *end = NULL;
        json_object *step = NULL, *is_pointer_slice = NULL;
        if (json_object_object_get_ex(expr, "is_pointer_slice", &is_pointer_slice) &&
            json_object_get_boolean(is_pointer_slice))
        {
            fprintf(stderr, "Error: Rust target does not support pointer array slices yet\n");
            return false;
        }
        if (json_object_object_get_ex(expr, "step", &step))
        {
            fprintf(stderr, "Error: Rust target does not support stepped array slices yet\n");
            return false;
        }
        if (!json_object_object_get_ex(expr, "array", &array) ||
            !rust_validate_expr(array)) return false;
        if (json_object_object_get_ex(expr, "start", &start) &&
            !rust_validate_expr(start)) return false;
        if (json_object_object_get_ex(expr, "end", &end) &&
            !rust_validate_expr(end)) return false;
        return true;
    }
    if (strcmp(kind, "index_assign") == 0)
    {
        json_object *array = NULL, *index = NULL, *value = NULL;
        return json_object_object_get_ex(expr, "array", &array) &&
               json_object_object_get_ex(expr, "index", &index) &&
               json_object_object_get_ex(expr, "value", &value) &&
               rust_validate_expr(array) && rust_validate_expr(index) &&
               rust_validate_expr(value);
    }
    if (strcmp(kind, "builtin_length") == 0)
        return json_object_object_get_ex(expr, "object", &child) &&
               rust_validate_expr(child);
    if (strcmp(kind, "member") == 0)
        return json_object_object_get_ex(expr, "object", &child) &&
               rust_validate_expr(child);
    if (strcmp(kind, "copy_of") == 0)
    {
        json_object *operand = NULL, *operand_type = NULL;
        json_object *element_type = NULL;
        const char *operand_kind = NULL;
        const char *operand_name = NULL;
        const char *user_copy_name = NULL;
        bool auto_copy_struct = false;
        if (!json_object_object_get_ex(expr, "operand", &operand) ||
            !json_object_object_get_ex(operand, "type", &operand_type) ||
            !(operand_kind = json_string_property(operand_type, "kind")))
        {
            fprintf(stderr,
                    "Error: Rust target encountered an invalid copyOf() operand\n");
            return false;
        }

        if (strcmp(operand_kind, "struct") == 0 &&
            json_string_property_equals(operand_type, "name", "TypeInfo"))
        {
            auto_copy_struct = true;
        }
        else if (strcmp(operand_kind, "struct") == 0)
        {
            operand_name = json_string_property(operand_type, "name");
            auto_copy_struct = rust_auto_copy_plain_value_struct_type(
                operand_type, &user_copy_name);
            if (user_copy_name)
            {
                if (operand_name && strcmp(operand_name, user_copy_name) == 0)
                    fprintf(stderr,
                            "Error: Rust target does not support copyOf() for value struct '%s' with a user-defined copy() method yet\n",
                            operand_name);
                else
                    fprintf(stderr,
                            "Error: Rust target does not support copyOf() for value struct '%s' because reachable value struct '%s' has a user-defined copy() method\n",
                            operand_name ? operand_name : "<anonymous>",
                            user_copy_name);
                return false;
            }
        }

        if (strcmp(operand_kind, "string") != 0 &&
            !auto_copy_struct &&
            (strcmp(operand_kind, "array") != 0 ||
             !json_object_object_get_ex(operand_type, "element_type", &element_type) ||
             !rust_array_copy_type_supported(element_type)))
        {
            fprintf(stderr,
                    "Error: Rust target currently supports copyOf() only for strings, auto-copy plain value structs, and arrays of integers, strings, booleans, characters, floating-point values, and heap-free named value structs\n");
            return false;
        }
        return rust_validate_expr(operand);
    }
    if (strcmp(kind, "static_call") == 0)
    {
        json_object *args = NULL;
        json_object_object_get_ex(expr, "args", &args);
        return rust_validate_expr_array(args);
    }
    if (strcmp(kind, "member_assign") == 0)
    {
        json_object *object = NULL, *value = NULL;
        return json_object_object_get_ex(expr, "object", &object) &&
               json_object_object_get_ex(expr, "value", &value) &&
               rust_validate_expr(object) && rust_validate_expr(value);
    }
    if (strcmp(kind, "binary") == 0)
    {
        json_object *left = NULL, *right = NULL;
        if (!json_object_object_get_ex(expr, "left", &left) ||
            !json_object_object_get_ex(expr, "right", &right)) return false;
        json_object *type = NULL;
        const char *type_kind = NULL;
        if (json_object_object_get_ex(expr, "type", &type) &&
            (type_kind = json_string_property(type, "kind")) &&
            strcmp(type_kind, "string") == 0)
        {
            json_object *left_type = NULL, *right_type = NULL;
            const char *left_kind = NULL, *right_kind = NULL;
            const char *op = json_string_property(expr, "op");
            if (!op || strcmp(op, "add") != 0 ||
                !json_object_object_get_ex(left, "type", &left_type) ||
                !json_object_object_get_ex(right, "type", &right_type) ||
                !(left_kind = json_string_property(left_type, "kind")) ||
                !(right_kind = json_string_property(right_type, "kind")) ||
                strcmp(left_kind, "string") != 0 || strcmp(right_kind, "string") != 0)
            {
                fprintf(stderr,
                        "Error: Rust target currently supports string concatenation only between strings\n");
                return false;
            }
        }
        return rust_validate_expr(left) && rust_validate_expr(right);
    }
    if (strcmp(kind, "str_concat_multi") == 0)
    {
        json_object *parts = NULL;
        if (!json_object_object_get_ex(expr, "parts", &parts)) return false;
        size_t count = json_object_array_length(parts);
        for (size_t i = 0; i < count; i++)
        {
            json_object *part = json_object_array_get_idx(parts, i);
            json_object *part_type = NULL;
            const char *part_kind = NULL;
            if (!json_object_object_get_ex(part, "type", &part_type) ||
                !(part_kind = json_string_property(part_type, "kind")) ||
                strcmp(part_kind, "string") != 0 || !rust_validate_expr(part))
            {
                fprintf(stderr,
                        "Error: Rust target currently supports string concatenation only between strings\n");
                return false;
            }
        }
        return true;
    }
    if (strcmp(kind, "compound_assign") == 0)
    {
        json_object *target = NULL, *value = NULL, *target_type = NULL, *value_type = NULL;
        const char *target_kind = NULL, *value_kind = NULL;
        if (!json_object_object_get_ex(expr, "target", &target) ||
            !json_object_object_get_ex(expr, "value", &value) ||
            !json_object_object_get_ex(target, "type", &target_type) ||
            !json_object_object_get_ex(value, "type", &value_type) ||
            !(target_kind = json_string_property(target_type, "kind")) ||
            !(value_kind = json_string_property(value_type, "kind")))
        {
            fprintf(stderr,
                    "Error: Rust target currently supports numeric compound assignment only between same-type integral operands\n");
            return false;
        }
        if (strcmp(target_kind, "string") == 0)
        {
            if (json_boolean_property(expr, "mutation_sync"))
            {
                fprintf(stderr,
                        "Error: Rust target does not support compound assignment for sync variables\n");
                return false;
            }
            if (strcmp(value_kind, "string") != 0 ||
                !json_string_property_equals(expr, "op", "add") ||
                !json_string_property_equals(target, "kind", "variable"))
            {
                fprintf(stderr,
                        "Error: Rust target currently supports += only for string variables and string values\n");
                return false;
            }
            return rust_validate_expr(target) && rust_validate_expr(value);
        }
        if (json_boolean_property(expr, "mutation_sync"))
        {
            fprintf(stderr,
                    "Error: Rust target does not support compound assignment for sync variables\n");
            return false;
        }
        if (json_string_property_equals(expr, "mutation_storage", "parameter") &&
            !json_string_property_equals(target, "parameter_mem_qual", "as_ref"))
        {
            fprintf(stderr,
                    "Error: Rust target does not support compound assignment of by-value parameters\n");
            return false;
        }
        if (json_string_property_equals(expr, "mutation_storage", "parameter") &&
            json_string_property_equals(target, "parameter_mem_qual", "as_ref") &&
            rust_floating_type(target_type))
        {
            fprintf(stderr,
                    "Error: Rust target does not support compound assignment of floating-point as ref parameters\n");
            return false;
        }
        bool target_floating = rust_floating_type(target_type);
        bool value_floating = rust_floating_type(value_type);
        if (target_floating || value_floating)
        {
            const char *op = json_string_property(expr, "op");
            if (!target_floating || !value_floating ||
                strcmp(target_kind, value_kind) != 0)
            {
                fprintf(stderr,
                        "Error: Rust target currently supports floating-point compound assignment only between same-type float or double operands\n");
                return false;
            }
            if (!op || (strcmp(op, "add") != 0 &&
                        strcmp(op, "subtract") != 0 &&
                        strcmp(op, "multiply") != 0 &&
                        strcmp(op, "divide") != 0))
            {
                fprintf(stderr,
                        "Error: Rust target supports floating-point compound assignment only for +=, -=, *=, and /=\n");
                return false;
            }
            if (!json_string_property_equals(expr, "mutation_place", "variable") &&
                !json_string_property_equals(expr, "mutation_place", "direct_field"))
            {
                fprintf(stderr,
                        "Error: Rust target supports floating-point compound assignment only for variables and direct fields\n");
                return false;
            }
            if (!json_string_property_equals(expr, "mutation_storage", "local"))
            {
                fprintf(stderr,
                        "Error: Rust target supports floating-point compound assignment only for stable mutable locals and direct fields\n");
                return false;
            }
            return rust_validate_expr(target) && rust_validate_expr(value);
        }
        bool checked_ref_parameter = rust_checked_scalar_ref_parameter(expr, target);
        if ((strcmp(target_kind, "int") != 0 && strcmp(target_kind, "long") != 0 &&
             strcmp(target_kind, "int32") != 0 && strcmp(target_kind, "uint") != 0 &&
             strcmp(target_kind, "uint32") != 0 && strcmp(target_kind, "byte") != 0) ||
            strcmp(target_kind, value_kind) != 0 ||
            !json_string_property_equals(expr, "mutation_arithmetic_mode", "checked") ||
            (!json_string_property_equals(expr, "mutation_storage", "local") &&
             !checked_ref_parameter))
        {
            fprintf(stderr,
                    "Error: Rust target currently supports numeric compound assignment only between same-type integral operands\n");
            return false;
        }
        if (!json_string_property_equals(expr, "mutation_place", "variable") &&
            !json_string_property_equals(expr, "mutation_place", "direct_field"))
        {
            fprintf(stderr,
                    "Error: Rust target supports checked numeric compound assignment only for variables and direct fields\n");
            return false;
        }
        return rust_validate_expr(target) && rust_validate_expr(value);
    }
    if (strcmp(kind, "unary") == 0)
        return json_object_object_get_ex(expr, "operand", &child) && rust_validate_expr(child);
    if (strcmp(kind, "increment") == 0 || strcmp(kind, "decrement") == 0)
    {
        const char *operand_kind = NULL;
        json_object *operand_type = NULL;
        if (!json_object_object_get_ex(expr, "operand", &child) ||
            !(operand_kind = json_string_property(child, "kind")) ||
            (strcmp(operand_kind, "variable") != 0 &&
             strcmp(operand_kind, "member") != 0) ||
            (!json_string_property_equals(expr, "mutation_place", "variable") &&
             !json_string_property_equals(expr, "mutation_place", "direct_field")))
        {
            fprintf(stderr,
                    "Error: Rust target supports increment/decrement only for variables and fields\n");
            return false;
        }
        if (!json_object_object_get_ex(child, "type", &operand_type))
        {
            fprintf(stderr,
                    "Error: Rust target supports increment/decrement only for variables and fields\n");
            return false;
        }
        bool operand_floating = rust_floating_type(operand_type);
        if (json_string_property_equals(expr, "mutation_storage", "parameter"))
        {
            if (!json_string_property_equals(child, "parameter_mem_qual", "as_ref"))
            {
                fprintf(stderr,
                        "Error: Rust target does not support increment/decrement of by-value parameters\n");
                return false;
            }
            if (operand_floating)
            {
                fprintf(stderr,
                        "Error: Rust target does not support increment/decrement of floating-point as ref parameters\n");
                return false;
            }
        }
        if (json_boolean_property(expr, "mutation_sync"))
        {
            fprintf(stderr,
                    "Error: Rust target does not support increment/decrement of sync variables\n");
            return false;
        }
        if (operand_floating)
        {
            if (!json_string_property_equals(expr, "mutation_storage", "local"))
            {
                fprintf(stderr,
                        "Error: Rust target supports floating-point increment/decrement only for stable mutable locals and direct fields\n");
                return false;
            }
            return rust_validate_expr(child);
        }
        if (!json_string_property_equals(expr, "mutation_arithmetic_mode", "checked"))
            return rust_validate_expr(child);
        if (!json_string_property_equals(expr, "mutation_storage", "local") &&
            !rust_checked_scalar_ref_parameter(expr, child))
        {
            fprintf(stderr,
                    "Error: Rust target supports checked increment/decrement only for local variables and direct fields\n");
            return false;
        }
        return rust_validate_expr(child);
    }
    if (strcmp(kind, "assign") == 0)
        return json_object_object_get_ex(expr, "value", &child) && rust_validate_expr(child);
    if (strcmp(kind, "call") == 0)
    {
        json_object *callee = NULL, *args = NULL, *callee_kind = NULL;
        if (!json_object_object_get_ex(expr, "callee", &callee) ||
            !json_object_object_get_ex(callee, "kind", &callee_kind))
            return false;
        const char *callee_kind_name = json_object_get_string(callee_kind);
        if (!callee_kind_name) return false;
        if (strcmp(callee_kind_name, "member") == 0)
        {
            json_object *object = NULL, *object_type = NULL;
            const char *object_type_kind = NULL;
            if (!json_object_object_get_ex(callee, "object", &object) ||
                !json_object_object_get_ex(object, "type", &object_type) ||
                !(object_type_kind = json_string_property(object_type, "kind"))) return false;
            const char *method = json_string_property(callee, "member_name");
            if (rust_primitive_conversion_member(object_type_kind, method))
            {
                json_object_object_get_ex(expr, "args", &args);
                if (!args || json_object_array_length(args) != 0) return false;
                if (!rust_validate_expr(object)) return false;
                if (!rust_primitive_integer_conversion_supported(object_type_kind, method))
                {
                    fprintf(stderr,
                            "Error: Rust target does not support primitive conversion %s.%s() yet\n",
                            object_type_kind, method);
                    return false;
                }
                json_object_object_add(expr, "rust_primitive_conversion",
                                       json_object_new_boolean(true));
                return true;
            }
            if (strcmp(object_type_kind, "array") == 0)
            {
                if (!rust_array_method_supported(method))
                {
                    fprintf(stderr, "Error: Rust target does not support array method '%s' yet\n",
                            method ? method : "<unknown>");
                    return false;
                }
                if ((strcmp(method, "contains") == 0 || strcmp(method, "indexOf") == 0))
                {
                    json_object *element_type = NULL, *arg = NULL, *arg_type = NULL;
                    const char *element_kind = NULL, *arg_kind = NULL;
                    if (!json_object_object_get_ex(object_type, "element_type", &element_type) ||
                        !(element_kind = json_string_property(element_type, "kind")) ||
                        !rust_array_search_type_supported(element_kind))
                    {
                        fprintf(stderr,
                                "Error: Rust target does not support array method '%s' for %s elements yet\n",
                                method, element_kind ? element_kind : "<unknown>");
                        return false;
                    }
                    if (strcmp(element_kind, "float") == 0 &&
                        (!json_object_object_get_ex(expr, "args", &args) ||
                         json_object_array_length(args) != 1 ||
                         !(arg = json_object_array_get_idx(args, 0)) ||
                         !json_object_object_get_ex(arg, "type", &arg_type) ||
                         !(arg_kind = json_string_property(arg_type, "kind")) ||
                         strcmp(arg_kind, "float") != 0))
                    {
                        fprintf(stderr,
                                "Error: Rust target requires array method '%s' on float[] to receive an exact float argument; got %s\n",
                                method, arg_kind ? arg_kind : "<unknown>");
                        return false;
                    }
                }
                if (strcmp(method, "concat") == 0)
                {
                    json_object *element_type = NULL;
                    const char *element_kind = NULL;
                    if (!json_object_object_get_ex(object_type, "element_type", &element_type) ||
                        !(element_kind = json_string_property(element_type, "kind")) ||
                        !rust_array_concat_type_supported(element_type))
                    {
                        fprintf(stderr,
                                "Error: Rust target does not support array method 'concat' for %s elements yet\n",
                                element_kind ? element_kind : "<unknown>");
                        return false;
                    }
                }
                if (strcmp(method, "join") == 0)
                {
                    json_object *element_type = NULL;
                    const char *element_kind = NULL;
                    if (!json_object_object_get_ex(object_type, "element_type", &element_type) ||
                        !(element_kind = json_string_property(element_type, "kind")) ||
                        strcmp(element_kind, "string") != 0)
                    {
                        fprintf(stderr,
                                "Error: Rust target does not support array method 'join' for %s elements yet\n",
                                element_kind ? element_kind : "<unknown>");
                        return false;
                    }
                }
                if (!rust_validate_expr(object))
                    return false;
            }
            else if (strcmp(object_type_kind, "string") == 0)
            {
                if (!rust_string_method_supported(method))
                {
                    fprintf(stderr, "Error: Rust target does not support string method '%s' yet\n",
                            method ? method : "<unknown>");
                    return false;
                }
                if (!rust_validate_expr(object)) return false;
            }
            else if (strcmp(object_type_kind, "struct") == 0)
            {
                if (!rust_validate_expr(object)) return false;
            }
            else if (strcmp(object_type_kind, "pointer") == 0)
            {
                json_object *base_type = NULL;
                if (!json_object_object_get_ex(object_type, "base_type", &base_type) ||
                    !json_string_property_equals(base_type, "kind", "struct") ||
                    !rust_validate_expr(object)) return false;
            }
            else return false;
        }
        else if (strcmp(callee_kind_name, "variable") != 0) return false;
        json_object_object_get_ex(expr, "args", &args);
        return rust_validate_expr_array(args);
    }
    if (strcmp(kind, "builtin_print") == 0 || strcmp(kind, "builtin_println") == 0)
    {
        json_object *args = NULL;
        json_object_object_get_ex(expr, "args", &args);
        return rust_validate_expr_array(args);
    }
    return false;
}

static bool rust_model_uses_arrays(json_object *node)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (rust_model_uses_arrays(json_object_array_get_idx(node, i))) return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;

    const char *kind = json_string_property(node, "kind");
    if (kind && (strcmp(kind, "array") == 0 || strcmp(kind, "array_literal") == 0 ||
                 strcmp(kind, "array_access") == 0 || strcmp(kind, "array_slice") == 0 ||
                 strcmp(kind, "index_assign") == 0 || strcmp(kind, "sized_array") == 0))
        return true;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (rust_model_uses_arrays(value)) return true;
    }
    return false;
}

static bool rust_model_uses_reflection(json_object *node)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (rust_model_uses_reflection(json_object_array_get_idx(node, i))) return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;

    if (json_string_property_equals(node, "kind", "typeof") ||
        (json_string_property_equals(node, "kind", "struct") &&
         (json_string_property_equals(node, "name", "TypeInfo") ||
          json_string_property_equals(node, "name", "FieldInfo"))))
        return true;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (rust_model_uses_reflection(value)) return true;
    }
    return false;
}

static bool rust_validate_stmt(json_object *stmt);

static bool rust_validate_statements(json_object *statements)
{
    if (!statements) return true;
    size_t count = json_object_array_length(statements);
    for (size_t i = 0; i < count; i++)
        if (!rust_validate_stmt(json_object_array_get_idx(statements, i))) return false;
    return true;
}

static bool rust_validate_block(json_object *block)
{
    json_object *statements = NULL;
    return block && json_object_object_get_ex(block, "statements", &statements) &&
           rust_validate_statements(statements);
}

static bool rust_int_match_literal_pattern(json_object *pattern)
{
    json_object *type = NULL;
    if (!json_object_is_type(pattern, json_type_object) ||
        !json_object_object_get_ex(pattern, "type", &type) ||
        !json_string_property_equals(type, "kind", "int")) return false;

    if (json_string_property_equals(pattern, "kind", "literal"))
    {
        json_object *value = NULL;
        return json_string_property_equals(pattern, "value_kind", "int") &&
               json_object_object_get_ex(pattern, "value", &value) &&
               json_object_is_type(value, json_type_int);
    }

    json_object *operand = NULL;
    if (!json_string_property_equals(pattern, "kind", "unary") ||
        !json_string_property_equals(pattern, "op", "negate") ||
        !json_object_object_get_ex(pattern, "operand", &operand) ||
        !json_string_property_equals(operand, "kind", "literal") ||
        !json_string_property_equals(operand, "value_kind", "int")) return false;

    json_object *operand_type = NULL, *value = NULL;
    return json_object_object_get_ex(operand, "type", &operand_type) &&
           json_string_property_equals(operand_type, "kind", "int") &&
           json_object_object_get_ex(operand, "value", &value) &&
           json_object_is_type(value, json_type_int);
}

static bool rust_bool_match_literal_pattern(json_object *pattern)
{
    json_object *type = NULL, *value = NULL;
    return json_object_is_type(pattern, json_type_object) &&
           json_object_object_get_ex(pattern, "type", &type) &&
           json_string_property_equals(type, "kind", "bool") &&
           json_string_property_equals(pattern, "kind", "literal") &&
           json_string_property_equals(pattern, "value_kind", "bool") &&
           json_object_object_get_ex(pattern, "value", &value) &&
           json_object_is_type(value, json_type_boolean);
}

static bool rust_validate_statement_match(json_object *expr)
{
    json_object *subject = NULL, *subject_type = NULL, *arms = NULL;
    if (!json_object_object_get_ex(expr, "subject", &subject) ||
        !json_object_is_type(subject, json_type_object) ||
        !json_object_object_get_ex(subject, "type", &subject_type) ||
        !json_object_object_get_ex(expr, "arms", &arms) ||
        !json_object_is_type(arms, json_type_array) ||
        json_object_array_length(arms) == 0)
    {
        return rust_report_match_error("encountered malformed statement match model");
    }
    bool subject_is_int = json_string_property_equals(subject_type, "kind", "int");
    bool subject_is_bool = json_string_property_equals(subject_type, "kind", "bool");
    if (!subject_is_int && !subject_is_bool)
    {
        return rust_report_match_error(
            "supports statement match only with int or bool subjects");
    }
    if (!rust_validate_expr(subject)) return false;

    bool has_else = false;
    bool has_pattern_arm = false;
    size_t arm_count = json_object_array_length(arms);
    for (size_t i = 0; i < arm_count; i++)
    {
        json_object *arm = json_object_array_get_idx(arms, i);
        json_object *is_else_obj = NULL, *patterns = NULL, *body = NULL;
        json_object *body_statements = NULL;
        if (!json_object_is_type(arm, json_type_object) ||
            !json_object_object_get_ex(arm, "is_else", &is_else_obj) ||
            !json_object_is_type(is_else_obj, json_type_boolean) ||
            !json_object_object_get_ex(arm, "patterns", &patterns) ||
            !json_object_is_type(patterns, json_type_array) ||
            !json_object_object_get_ex(arm, "body", &body) ||
            !json_string_property_equals(body, "kind", "block") ||
            !json_object_object_get_ex(body, "statements", &body_statements) ||
            !json_object_is_type(body_statements, json_type_array))
        {
            return rust_report_match_error(
                "encountered malformed statement match model");
        }

        bool is_else = json_object_get_boolean(is_else_obj);
        size_t pattern_count = json_object_array_length(patterns);
        if ((is_else && (has_else || i + 1 != arm_count || pattern_count != 0)) ||
            (!is_else && pattern_count == 0))
        {
            return rust_report_match_error(
                "encountered malformed statement match model");
        }
        if (is_else)
        {
            has_else = true;
        }
        else
        {
            has_pattern_arm = true;
            for (size_t p = 0; p < pattern_count; p++)
            {
                json_object *pattern = json_object_array_get_idx(patterns, p);
                if (subject_is_int && !rust_int_match_literal_pattern(pattern))
                    return rust_report_match_error(
                        "supports statement match only with integer literal patterns");
                if (subject_is_bool && !rust_bool_match_literal_pattern(pattern))
                    return rust_report_match_error(
                        "supports statement match only with boolean literal patterns");
            }
        }

        if (!rust_validate_block(body)) return false;
    }

    if (!has_pattern_arm)
    {
        return rust_report_match_error(
            "encountered malformed statement match model");
    }

    json_object_object_add(expr, "rust_has_else", json_object_new_boolean(has_else));
    return true;
}

static bool rust_validate_value_match(json_object *expr)
{
    json_object *result_type = NULL, *subject = NULL, *subject_type = NULL;
    json_object *arms = NULL;
    if (!json_object_object_get_ex(expr, "type", &result_type) ||
        !json_object_object_get_ex(expr, "subject", &subject) ||
        !json_object_is_type(subject, json_type_object) ||
        !json_object_object_get_ex(subject, "type", &subject_type) ||
        !json_object_object_get_ex(expr, "arms", &arms) ||
        !json_object_is_type(arms, json_type_array) ||
        json_object_array_length(arms) == 0)
        return rust_report_match_error("encountered malformed value match model");

    if (!json_string_property_equals(subject_type, "kind", "int"))
        return rust_report_match_error(
            "supports value match only with int subjects");
    if (!rust_validate_expr(subject)) return false;

    size_t else_count = 0;
    size_t ordinary_count = 0;
    size_t arm_count = json_object_array_length(arms);
    for (size_t i = 0; i < arm_count; i++)
    {
        json_object *arm = json_object_array_get_idx(arms, i);
        json_object *is_else_obj = NULL, *patterns = NULL, *body = NULL;
        json_object *body_statements = NULL;
        if (!json_object_is_type(arm, json_type_object) ||
            !json_object_object_get_ex(arm, "is_else", &is_else_obj) ||
            !json_object_is_type(is_else_obj, json_type_boolean) ||
            !json_object_object_get_ex(arm, "patterns", &patterns) ||
            !json_object_is_type(patterns, json_type_array) ||
            !json_object_object_get_ex(arm, "body", &body) ||
            !json_string_property_equals(body, "kind", "block") ||
            !json_object_object_get_ex(body, "statements", &body_statements) ||
            !json_object_is_type(body_statements, json_type_array))
            return rust_report_match_error("encountered malformed value match model");

        bool is_else = json_object_get_boolean(is_else_obj);
        size_t pattern_count = json_object_array_length(patterns);
        if (is_else)
        {
            else_count++;
            if (pattern_count != 0)
                return rust_report_match_error("encountered malformed value match model");
        }
        else
        {
            ordinary_count++;
            if (pattern_count == 0)
                return rust_report_match_error("encountered malformed value match model");
            for (size_t p = 0; p < pattern_count; p++)
            {
                if (!rust_int_match_literal_pattern(
                        json_object_array_get_idx(patterns, p)))
                    return rust_report_match_error(
                        "supports value match only with integer literal patterns");
            }
        }
    }

    if (ordinary_count == 0)
        return rust_report_match_error(
            "requires value match to contain at least one ordinary arm");
    if (else_count != 1 ||
        !json_boolean_property(json_object_array_get_idx(arms, arm_count - 1),
                               "is_else"))
        return rust_report_match_error(
            "requires value match to contain exactly one final else arm");
    if (!json_string_property_equals(result_type, "kind", "int"))
        return rust_report_match_error("supports value match only with int results");

    for (size_t i = 0; i < arm_count; i++)
    {
        json_object *arm = json_object_array_get_idx(arms, i);
        json_object *body = NULL, *body_statements = NULL;
        if (!json_object_object_get_ex(arm, "body", &body) ||
            !json_object_object_get_ex(body, "statements", &body_statements) ||
            json_object_array_length(body_statements) != 1)
            return rust_report_match_error(
                "requires each value match arm body to contain exactly one int expression");

        json_object *statement = json_object_array_get_idx(body_statements, 0);
        json_object *arm_expr = NULL, *arm_type = NULL;
        if (!json_string_property_equals(statement, "kind", "expr") ||
            !json_object_object_get_ex(statement, "expr", &arm_expr) ||
            !json_object_is_type(arm_expr, json_type_object) ||
            !json_object_object_get_ex(arm_expr, "type", &arm_type) ||
            !json_string_property_equals(arm_type, "kind", "int"))
            return rust_report_match_error(
                "requires each value match arm body to contain exactly one int expression");
        if (!rust_validate_expr(arm_expr)) return false;
    }

    json_object_object_add(expr, "rust_value_match", json_object_new_boolean(true));
    return true;
}

static bool rust_iterator_scalar_element_supported(json_object *type)
{
    const char *kind = json_string_property(type, "kind");
    return kind && (rust_integer_type(kind) || rust_float_type(kind) ||
                    strcmp(kind, "bool") == 0 || strcmp(kind, "char") == 0);
}

static bool rust_validate_for_each_iter(json_object *stmt)
{
    json_object *iterable = NULL, *body = NULL, *iterable_type = NULL;
    json_object *iter_type = NULL, *element_type = NULL;
    if (!json_object_object_get_ex(stmt, "iterable", &iterable) ||
        !json_object_object_get_ex(stmt, "body", &body) ||
        !json_object_object_get_ex(stmt, "iter_type", &iter_type) ||
        !json_object_object_get_ex(stmt, "element_type", &element_type) ||
        !json_object_object_get_ex(iterable, "type", &iterable_type))
    {
        fprintf(stderr,
                "Error: Rust target encountered malformed iterator-protocol foreach model\n");
        return false;
    }

    const char *element_kind = json_string_property(element_type, "kind");
    if (!rust_iterator_scalar_element_supported(element_type) ||
        !json_string_property_equals(stmt, "element_cleanup_kind", "none"))
    {
        fprintf(stderr,
                "Error: Rust target supports iterator-protocol foreach only for heap-free scalar elements; got '%s'\n",
                element_kind ? element_kind : "<unknown>");
        return false;
    }

    if (json_boolean_property(stmt, "iterable_pass_by_ref") ||
        json_boolean_property(stmt, "iter_pass_by_ref") ||
        !json_string_property_equals(stmt, "iter_cleanup_kind", "none") ||
        !rust_heap_free_named_struct_type(iterable_type) ||
        !rust_heap_free_named_struct_type(iter_type))
    {
        fprintf(stderr,
                "Error: Rust target supports iterator-protocol foreach only with plain heap-free value iterable and iterator structs\n");
        return false;
    }

    return rust_validate_expr(iterable) && rust_validate_block(body);
}

static bool rust_validate_stmt(json_object *stmt)
{
    json_object *kind_obj = NULL;
    if (!stmt || !json_object_object_get_ex(stmt, "kind", &kind_obj)) return false;
    const char *kind = json_object_get_string(kind_obj);
    json_object *child = NULL;
    if (!kind) return false;
    if (strcmp(kind, "break") == 0 || strcmp(kind, "continue") == 0) return true;
    if (strcmp(kind, "return") == 0)
        return !json_object_object_get_ex(stmt, "value", &child) || rust_validate_expr(child);
    if (strcmp(kind, "expr") == 0)
    {
        if (!json_object_object_get_ex(stmt, "expr", &child)) return false;
        if (json_string_property_equals(child, "kind", "match"))
            return rust_validate_statement_match(child);
        return rust_validate_expr(child);
    }
    if (strcmp(kind, "var_decl") == 0)
    {
        json_object *type = NULL;
        if (!json_object_object_get_ex(stmt, "type", &type) || !rust_type_supported(type)) return false;
        if (json_object_object_get_ex(stmt, "initializer", &child))
            return rust_validate_expr(child);
        const char *type_kind = json_string_property(type, "kind");
        return !type_kind || strcmp(type_kind, "struct") != 0;
    }
    if (strcmp(kind, "block") == 0) return rust_validate_block(stmt);
    if (strcmp(kind, "while") == 0)
    {
        json_object *condition = NULL, *body = NULL;
        return json_object_object_get_ex(stmt, "condition", &condition) &&
               json_object_object_get_ex(stmt, "body", &body) &&
               rust_validate_expr(condition) && rust_validate_block(body);
    }
    if (strcmp(kind, "for") == 0)
    {
        json_object *init = NULL, *condition = NULL, *increment = NULL, *body = NULL;
        return json_object_object_get_ex(stmt, "init", &init) &&
               json_object_object_get_ex(stmt, "condition", &condition) &&
               json_object_object_get_ex(stmt, "increment", &increment) &&
               json_object_object_get_ex(stmt, "body", &body) &&
               rust_validate_stmt(init) && rust_validate_expr(condition) &&
               rust_validate_expr(increment) && rust_validate_block(body);
    }
    if (strcmp(kind, "for_each") == 0)
    {
        json_object *iterable = NULL, *body = NULL;
        return json_object_object_get_ex(stmt, "iterable", &iterable) &&
               json_object_object_get_ex(stmt, "body", &body) &&
               rust_validate_expr(iterable) && rust_validate_block(body);
    }
    if (strcmp(kind, "for_each_iter") == 0)
        return rust_validate_for_each_iter(stmt);
    if (strcmp(kind, "if") == 0)
    {
        json_object *condition = NULL, *then_body = NULL, *else_body = NULL;
        if (!json_object_object_get_ex(stmt, "condition", &condition) ||
            !json_object_object_get_ex(stmt, "then_body", &then_body) ||
            !rust_validate_expr(condition) || !rust_validate_block(then_body)) return false;
        return !json_object_object_get_ex(stmt, "else_body", &else_body) ||
               rust_validate_block(else_body);
    }
    return false;
}

/* A method needs &mut self only when its mutation place is rooted in self.
 * Local values (including their fields, indices, and arrays) must not turn an
 * otherwise read-only instance method into a mutable receiver method. */
static bool rust_mutation_place_is_self_rooted(json_object *place)
{
    if (!json_object_is_type(place, json_type_object)) return false;
    if (json_string_property_equals(place, "kind", "variable"))
        return json_string_property_equals(place, "name", "self");

    json_object *parent = NULL;
    if (json_string_property_equals(place, "kind", "member") &&
        json_object_object_get_ex(place, "object", &parent))
        return rust_mutation_place_is_self_rooted(parent);
    if (json_string_property_equals(place, "kind", "array_access") &&
        json_object_object_get_ex(place, "array", &parent))
        return rust_mutation_place_is_self_rooted(parent);
    return false;
}

static bool rust_is_mutating_array_call(json_object *node)
{
    if (!json_string_property_equals(node, "kind", "call")) return false;
    json_object *callee = NULL, *object = NULL, *type = NULL;
    if (!json_object_object_get_ex(node, "callee", &callee) ||
        !json_string_property_equals(callee, "kind", "member") ||
        !json_object_object_get_ex(callee, "object", &object) ||
        !json_object_object_get_ex(object, "type", &type) ||
        !json_string_property_equals(type, "kind", "array")) return false;
    const char *method = json_string_property(callee, "member_name");
    return method && (strcmp(method, "push") == 0 ||
                      strcmp(method, "pop") == 0 ||
                      strcmp(method, "insert") == 0 ||
                      strcmp(method, "remove") == 0 ||
                      strcmp(method, "reverse") == 0 ||
                      strcmp(method, "clear") == 0);
}

static bool rust_instance_method_node_supported(json_object *node,
                                                bool allow_owned_self)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (!rust_instance_method_node_supported(
                    json_object_array_get_idx(node, i), allow_owned_self)) return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;

    const char *kind = json_string_property(node, "kind");
    if (kind && strcmp(kind, "variable") == 0)
        return allow_owned_self || !json_string_property_equals(node, "name", "self");
    if (kind && strcmp(kind, "member_assign") == 0)
    {
        json_object *object = NULL, *value = NULL;
        if (!json_object_object_get_ex(node, "object", &object) ||
            !json_object_object_get_ex(node, "value", &value) ||
            !rust_instance_method_node_supported(value, allow_owned_self)) return false;
        if (json_string_property_equals(object, "kind", "variable") &&
            json_string_property_equals(object, "name", "self")) return true;
        return rust_instance_method_node_supported(object, allow_owned_self);
    }
    if (kind && strcmp(kind, "member") == 0)
    {
        json_object *object = NULL;
        if (!json_object_object_get_ex(node, "object", &object)) return false;
        if (json_string_property_equals(object, "kind", "variable") &&
            json_string_property_equals(object, "name", "self")) return true;
        return rust_instance_method_node_supported(object, allow_owned_self);
    }
    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (!rust_instance_method_node_supported(value, allow_owned_self)) return false;
    }
    return true;
}

static bool rust_method_has_direct_mutation(json_object *node)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (rust_method_has_direct_mutation(
                    json_object_array_get_idx(node, i))) return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;
    json_object *place = NULL;
    if ((json_string_property_equals(node, "kind", "compound_assign") &&
         json_object_object_get_ex(node, "target", &place)) ||
        ((json_string_property_equals(node, "kind", "increment") ||
          json_string_property_equals(node, "kind", "decrement")) &&
         json_object_object_get_ex(node, "operand", &place)) ||
        (json_string_property_equals(node, "kind", "member_assign") &&
         json_object_object_get_ex(node, "object", &place)) ||
        (json_string_property_equals(node, "kind", "index_assign") &&
         json_object_object_get_ex(node, "array", &place)))
    {
        if (rust_mutation_place_is_self_rooted(place)) return true;
    }
    if (rust_is_mutating_array_call(node))
    {
        json_object *callee = NULL, *object = NULL;
        if (json_object_object_get_ex(node, "callee", &callee) &&
            json_object_object_get_ex(callee, "object", &object) &&
            rust_mutation_place_is_self_rooted(object)) return true;
    }
    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (rust_method_has_direct_mutation(value)) return true;
    }
    return false;
}

static bool rust_method_calls_mutating_self(json_object *node,
                                            json_object *methods)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (rust_method_calls_mutating_self(
                    json_object_array_get_idx(node, i), methods)) return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;

    if (json_string_property_equals(node, "kind", "call"))
    {
        json_object *callee = NULL, *object = NULL;
        if (json_object_object_get_ex(node, "callee", &callee) &&
            json_string_property_equals(callee, "kind", "member") &&
            json_object_object_get_ex(callee, "object", &object) &&
            json_string_property_equals(object, "kind", "variable") &&
            json_string_property_equals(object, "name", "self"))
        {
            const char *called_name = json_string_property(callee, "member_name");
            size_t method_count = json_object_array_length(methods);
            for (size_t i = 0; called_name && i < method_count; i++)
            {
                json_object *called = json_object_array_get_idx(methods, i);
                const char *name = json_string_property(called, "name");
                if (name && strcmp(name, called_name) == 0 &&
                    json_boolean_property(called, "rust_mutating")) return true;
            }
        }
    }

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (rust_method_calls_mutating_self(value, methods)) return true;
    }
    return false;
}

static bool rust_validate_struct_methods(json_object *model)
{
    json_object *structs = NULL;
    if (!json_object_object_get_ex(model, "structs", &structs)) return true;

    size_t struct_count = json_object_array_length(structs);
    for (size_t i = 0; i < struct_count; i++)
    {
        json_object *structure = json_object_array_get_idx(structs, i);
        const char *struct_name = json_string_property(structure, "name");
        json_object *methods = NULL;
        if (!json_object_object_get_ex(structure, "methods", &methods)) continue;

        size_t method_count = json_object_array_length(methods);
        for (size_t m = 0; m < method_count; m++)
        {
            json_object *method = json_object_array_get_idx(methods, m);
            json_object *body = NULL;
            if (!json_boolean_property(method, "is_static") &&
                json_object_object_get_ex(method, "body", &body) &&
                rust_method_has_direct_mutation(body))
                json_object_object_add(method, "rust_mutating",
                                       json_object_new_boolean(true));
        }
        bool changed;
        do
        {
            changed = false;
            for (size_t m = 0; m < method_count; m++)
            {
                json_object *method = json_object_array_get_idx(methods, m);
                json_object *body = NULL;
                if (!json_boolean_property(method, "is_static") &&
                    !json_boolean_property(method, "rust_mutating") &&
                    json_object_object_get_ex(method, "body", &body) &&
                    rust_method_calls_mutating_self(body, methods))
                {
                    json_object_object_add(method, "rust_mutating",
                                           json_object_new_boolean(true));
                    changed = true;
                }
            }
        }
        while (changed);

        for (size_t m = 0; m < method_count; m++)
        {
            json_object *method = json_object_array_get_idx(methods, m);
            const char *method_name = json_string_property(method, "name");
            json_object *return_type = NULL, *params = NULL, *body = NULL;
            bool is_static = json_boolean_property(method, "is_static");
            if (json_boolean_property(method, "is_native"))
            {
                fprintf(stderr,
                        "Error: Rust target supports only non-native methods on plain value struct '%s'\n",
                        struct_name ? struct_name : "<anonymous>");
                return false;
            }
            if (!json_object_object_get_ex(method, "return_type", &return_type) ||
                !rust_type_supported(return_type))
            {
                fprintf(stderr,
                        "Error: Rust target does not support return type of static method '%s.%s'\n",
                        struct_name ? struct_name : "<anonymous>",
                        method_name ? method_name : "<anonymous>");
                return false;
            }
            if (json_object_object_get_ex(method, "params", &params))
            {
                size_t param_count = json_object_array_length(params);
                for (size_t p = 0; p < param_count; p++)
                {
                    json_object *param = json_object_array_get_idx(params, p);
                    json_object *param_type = NULL;
                    const char *mem_qual = json_string_property(param, "mem_qual");
                    const char *sync_mod = json_string_property(param, "sync_mod");
                    bool has_param_type =
                        json_object_object_get_ex(param, "type", &param_type);
                    bool mem_qual_supported =
                        !mem_qual || strcmp(mem_qual, "default") == 0 ||
                        (has_param_type &&
                         strcmp(mem_qual, "as_ref") == 0 &&
                         rust_scalar_ref_parameter_type_supported(param_type)) ||
                        (is_static && has_param_type &&
                         strcmp(mem_qual, "as_ref") == 0 &&
                         rust_heap_free_named_struct_type(param_type)) ||
                        (is_static && has_param_type &&
                         strcmp(mem_qual, "as_val") == 0 &&
                         rust_heap_free_named_struct_type(param_type)) ||
                        (!is_static &&
                         !json_boolean_property(method, "rust_mutating") &&
                         has_param_type && strcmp(mem_qual, "as_val") == 0 &&
                         rust_heap_free_named_struct_type(param_type));
                    if (!has_param_type ||
                        !rust_type_supported(param_type) ||
                        !mem_qual_supported ||
                        (sync_mod && strcmp(sync_mod, "none") != 0))
                    {
                        fprintf(stderr,
                                "Error: Rust target does not support a parameter of %smethod '%s.%s'\n",
                                is_static ? "static " : "",
                                struct_name ? struct_name : "<anonymous>",
                                method_name ? method_name : "<anonymous>");
                        return false;
                    }
                }
            }
            json_object_object_get_ex(method, "body", &body);
            if (!rust_validate_statements(body) ||
                (!is_static && !rust_instance_method_node_supported(
                    body, !json_boolean_property(structure, "has_heap_fields"))))
            {
                if (!rust_validation_reported_error)
                    fprintf(stderr,
                            "Error: Rust target encountered an unsupported construct in method '%s.%s'\n",
                            struct_name ? struct_name : "<anonymous>",
                            method_name ? method_name : "<anonymous>");
                return false;
            }
        }
    }
    return true;
}

static bool rust_validate_model_impl(json_object *model)
{
    const char *unsupported = NULL;
    if (!array_is_empty(model, "globals")) unsupported = "global variables";
    else if (!array_is_empty(model, "lambdas")) unsupported = "closures";
    else if (!array_is_empty(model, "threads")) unsupported = "threads";
    else if (!array_is_empty(model, "type_decls")) unsupported = "type declarations";

    json_object *pragmas = NULL;
    if (!unsupported && json_object_object_get_ex(model, "pragmas", &pragmas))
    {
        size_t count = json_object_array_length(pragmas);
        for (size_t i = 0; i < count; i++)
        {
            json_object *pragma = json_object_array_get_idx(pragmas, i);
            json_object *kind = NULL;
            if (json_object_object_get_ex(pragma, "pragma_type", &kind))
            {
                const char *value = json_object_get_string(kind);
                if (value && (strcmp(value, "source") == 0 || strcmp(value, "include") == 0))
                {
                    unsupported = "native C source/include pragmas";
                    break;
                }
            }
        }
    }

    if (unsupported)
    {
        fprintf(stderr, "Error: Rust target does not support %s yet\n", unsupported);
        return false;
    }

    if (!rust_validate_structs(model) ||
        !rust_validate_struct_methods(model)) return false;

    json_object *functions = NULL;
    if (json_object_object_get_ex(model, "functions", &functions))
    {
        size_t count = json_object_array_length(functions);
        for (size_t i = 0; i < count; i++)
        {
            json_object *function = json_object_array_get_idx(functions, i);
            json_object *name_obj = NULL, *return_type = NULL, *params = NULL, *body = NULL;
            json_object *is_native = NULL;
            const char *name = json_object_object_get_ex(function, "name", &name_obj)
                ? json_object_get_string(name_obj) : "<anonymous>";
            if ((json_object_object_get_ex(function, "is_native", &is_native) &&
                 json_object_get_boolean(is_native)) ||
                !json_object_object_get_ex(function, "return_type", &return_type) ||
                !rust_type_supported(return_type))
            {
                fprintf(stderr, "Error: Rust target does not support function '%s' yet\n", name);
                return false;
            }
            json_object *return_kind = NULL;
            if (strcmp(name, "main") == 0 &&
                json_object_object_get_ex(return_type, "kind", &return_kind) &&
                strcmp(json_object_get_string(return_kind), "void") != 0 &&
                strcmp(json_object_get_string(return_kind), "int") != 0)
            {
                fprintf(stderr, "Error: Rust target requires main to return void or int\n");
                return false;
            }
            if (strcmp(name, "main") == 0 &&
                json_object_object_get_ex(return_type, "kind", &return_kind) &&
                strcmp(json_object_get_string(return_kind), "int") == 0)
                json_object_object_add(function, "rust_main_returns_int",
                                       json_object_new_boolean(true));
            if (strcmp(name, "main") == 0 &&
                json_object_object_get_ex(function, "params", &params) &&
                json_object_array_length(params) > 0)
            {
                size_t param_count = json_object_array_length(params);
                json_object *param = json_object_array_get_idx(params, 0);
                json_object *param_type = NULL;
                json_object *element_type = NULL;
                const char *param_name = json_string_property(param, "name");
                if (param_count != 1 ||
                    !json_object_object_get_ex(param, "type", &param_type) ||
                    !json_string_property_equals(param_type, "kind", "array") ||
                    !json_object_object_get_ex(param_type, "element_type", &element_type) ||
                    !json_string_property_equals(element_type, "kind", "string"))
                {
                    fprintf(stderr,
                            "Error: Rust target requires main to have zero parameters or a single str[] parameter\n");
                    return false;
                }
                json_object_object_add(function, "rust_main_has_args",
                                       json_object_new_boolean(true));
                if (param_name)
                    json_object_object_add(function, "rust_main_args_name",
                                           json_object_new_string(param_name));
            }
            if (json_object_object_get_ex(function, "params", &params))
            {
                size_t param_count = json_object_array_length(params);
                for (size_t p = 0; p < param_count; p++)
                {
                    json_object *param = json_object_array_get_idx(params, p);
                    json_object *param_type = NULL;
                    const char *mem_qual = json_string_property(param, "mem_qual");
                    const char *sync_mod = json_string_property(param, "sync_mod");
                    bool has_param_type =
                        json_object_object_get_ex(param, "type", &param_type);
                    bool mem_qual_supported =
                        !mem_qual || strcmp(mem_qual, "default") == 0 ||
                        (has_param_type && strcmp(mem_qual, "as_ref") == 0 &&
                         (rust_heap_free_named_struct_type(param_type) ||
                          rust_scalar_ref_parameter_type_supported(param_type))) ||
                        (has_param_type && strcmp(mem_qual, "as_val") == 0 &&
                         rust_heap_free_named_struct_type(param_type));
                    if (!has_param_type ||
                        !rust_type_supported(param_type) ||
                        !mem_qual_supported ||
                        (sync_mod && strcmp(sync_mod, "none") != 0))
                    {
                        fprintf(stderr, "Error: Rust target does not support a parameter of function '%s'\n", name);
                        return false;
                    }
                }
            }
            json_object_object_get_ex(function, "body", &body);
            if (!rust_validate_statements(body))
            {
                if (!rust_validation_reported_error)
                    fprintf(stderr, "Error: Rust target encountered an unsupported construct in function '%s'\n", name);
                return false;
            }
        }
    }
    return true;
}

static bool rust_validate_model(json_object *model)
{
    rust_validation_model = model;
    rust_validation_reported_error = false;
    bool valid = rust_validate_model_impl(model);
    rust_validation_model = NULL;
    rust_validation_reported_error = false;
    return valid;
}

/* Annotate target-neutral binary nodes with the Rust checked-arithmetic method
 * selected by this backend. Templates remain declarative and other targets do
 * not need to understand Rust's checked_* APIs. */
static void rust_lower_checked_arithmetic(json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_lower_checked_arithmetic(json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_lower_checked_arithmetic(value);
    }

    json_object *kind_obj = NULL, *mode_obj = NULL, *op_obj = NULL, *type_obj = NULL;
    if (!json_object_object_get_ex(node, "kind", &kind_obj) ||
        strcmp(json_object_get_string(kind_obj), "binary") != 0 ||
        !json_object_object_get_ex(node, "arithmetic_mode", &mode_obj) ||
        strcmp(json_object_get_string(mode_obj), "checked") != 0 ||
        !json_object_object_get_ex(node, "op", &op_obj) ||
        !json_object_object_get_ex(node, "type", &type_obj))
        return;

    json_object *type_kind_obj = NULL;
    if (!json_object_object_get_ex(type_obj, "kind", &type_kind_obj)) return;
    const char *type_kind = json_object_get_string(type_kind_obj);
    if (!type_kind || (strcmp(type_kind, "int") != 0 && strcmp(type_kind, "long") != 0 &&
        strcmp(type_kind, "int32") != 0 && strcmp(type_kind, "uint") != 0 &&
        strcmp(type_kind, "uint32") != 0 && strcmp(type_kind, "byte") != 0))
        return;

    const char *op = json_object_get_string(op_obj);
    const char *method = NULL;
    if (strcmp(op, "add") == 0) method = "checked_add";
    else if (strcmp(op, "subtract") == 0) method = "checked_sub";
    else if (strcmp(op, "multiply") == 0) method = "checked_mul";
    else if (strcmp(op, "divide") == 0) method = "checked_div";
    else if (strcmp(op, "modulo") == 0) method = "checked_rem";
    if (method)
        json_object_object_add(node, "rust_checked_method", json_object_new_string(method));
}

/* Apply the shared mutation annotations after validation.  A checked mutation
 * is represented as a borrowed place plus a checked_* method in Rust, rather
 * than reconstructing an arithmetic expression in a template. */
static void rust_lower_checked_mutations(json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_lower_checked_mutations(json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_lower_checked_mutations(value);
    }

    const char *kind = json_string_property(node, "kind");
    const char *mode = json_string_property(node, "mutation_arithmetic_mode");
    const char *op = json_string_property(node, "mutation_op");
    if (!kind || !mode || !op || strcmp(mode, "checked") != 0 ||
        (strcmp(kind, "compound_assign") != 0 && strcmp(kind, "increment") != 0 &&
         strcmp(kind, "decrement") != 0))
        return;

    const char *method = NULL;
    if (strcmp(op, "add") == 0) method = "checked_add";
    else if (strcmp(op, "subtract") == 0) method = "checked_sub";
    else if (strcmp(op, "multiply") == 0) method = "checked_mul";
    else if (strcmp(op, "divide") == 0) method = "checked_div";
    else if (strcmp(op, "modulo") == 0) method = "checked_rem";
    if (method)
        json_object_object_add(node, "rust_checked_method", json_object_new_string(method));
}

/* Floating compound assignment and postfix mutation use the same shared
 * stable-place annotations as checked integer mutation, but ordinary Rust
 * f32/f64 arithmetic. */
static void rust_lower_floating_mutations(json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_lower_floating_mutations(json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_lower_floating_mutations(value);
    }

    json_object *target = NULL, *type = NULL;
    const char *kind = json_string_property(node, "kind");
    const char *op = json_string_property(node, "op");
    if (kind && strcmp(kind, "compound_assign") == 0)
    {
        if (!json_object_object_get_ex(node, "target", &target) ||
            !json_object_object_get_ex(target, "type", &type) ||
            !rust_floating_type(type) || !op ||
            (strcmp(op, "add") != 0 && strcmp(op, "subtract") != 0 &&
             strcmp(op, "multiply") != 0 && strcmp(op, "divide") != 0))
            return;

        json_object_object_add(node, "rust_floating_compound",
                               json_object_new_boolean(true));
        return;
    }

    const char *mutation_op = json_string_property(node, "mutation_op");
    if ((!kind || (strcmp(kind, "increment") != 0 &&
                   strcmp(kind, "decrement") != 0)) ||
        !json_object_object_get_ex(node, "operand", &target) ||
        !json_object_object_get_ex(target, "type", &type) ||
        !rust_floating_type(type) || !mutation_op ||
        (strcmp(mutation_op, "add") != 0 &&
         strcmp(mutation_op, "subtract") != 0))
        return;

    json_object_object_add(node, "rust_floating_postfix",
                           json_object_new_boolean(true));
}

/* Mark string constructs after shared model generation so Rust syntax choices
 * stay within this backend. */
static void rust_lower_strings(json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_lower_strings(json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_lower_strings(value);
    }

    const char *kind = json_string_property(node, "kind");
    if (!kind) return;
    if (strcmp(kind, "binary") == 0)
    {
        json_object *type = NULL;
        if (json_object_object_get_ex(node, "type", &type) &&
            json_string_property_equals(type, "kind", "string") &&
            json_string_property_equals(node, "op", "add"))
            json_object_object_add(node, "rust_string_concat", json_object_new_boolean(true));
        return;
    }
    if (strcmp(kind, "compound_assign") == 0)
    {
        json_object *target = NULL, *type = NULL;
        if (json_object_object_get_ex(node, "target", &target) &&
            json_object_object_get_ex(target, "type", &type) &&
            json_string_property_equals(type, "kind", "string"))
            json_object_object_add(node, "rust_string_append", json_object_new_boolean(true));
        return;
    }
    if (strcmp(kind, "call") == 0)
    {
        json_object *callee = NULL, *object = NULL, *type = NULL;
        if (json_object_object_get_ex(node, "callee", &callee) &&
            json_string_property_equals(callee, "kind", "member") &&
            json_object_object_get_ex(callee, "object", &object) &&
            json_object_object_get_ex(object, "type", &type) &&
            json_string_property_equals(type, "kind", "string"))
        {
            const char *method = json_string_property(callee, "member_name");
            if (method)
                json_object_object_add(node, "rust_string_method",
                                       json_object_new_string(method));
        }

        /* Sindarin passes owned strings by value without consuming an lvalue at
         * the call site. C's model does not need an acquire annotation for all
         * default parameters, so record the Rust move/clone decision here. */
        bool copies_owned_args = false;
        if (json_object_object_get_ex(node, "callee", &callee))
        {
            copies_owned_args = json_string_property_equals(callee, "kind", "variable");
            if (!copies_owned_args &&
                json_string_property_equals(callee, "kind", "member") &&
                json_object_object_get_ex(callee, "object", &object) &&
                json_object_object_get_ex(object, "type", &type))
            {
                copies_owned_args = json_string_property_equals(type, "kind", "struct");
                if (json_string_property_equals(type, "kind", "pointer"))
                {
                    json_object *base_type = NULL;
                    copies_owned_args =
                        json_object_object_get_ex(type, "base_type", &base_type) &&
                        json_string_property_equals(base_type, "kind", "struct");
                }
            }
        }
        if (copies_owned_args)
        {
            json_object *args = NULL;
            if (json_object_object_get_ex(node, "args", &args))
            {
                size_t count = json_object_array_length(args);
                for (size_t i = 0; i < count; i++)
                {
                    json_object *arg = json_object_array_get_idx(args, i);
                    json_object *arg_type = NULL;
                    const char *arg_kind = json_string_property(arg, "kind");
                    if (json_object_object_get_ex(arg, "type", &arg_type) &&
                        json_string_property_equals(arg_type, "kind", "string") &&
                        arg_kind && (strcmp(arg_kind, "variable") == 0 ||
                                     strcmp(arg_kind, "member") == 0 ||
                                     strcmp(arg_kind, "array_access") == 0))
                        json_object_object_add(arg, "rust_needs_clone",
                                               json_object_new_boolean(true));
                }
            }
        }
    }
    else if (strcmp(kind, "static_call") == 0)
    {
        json_object *args = NULL;
        if (json_object_object_get_ex(node, "args", &args))
        {
            size_t count = json_object_array_length(args);
            for (size_t i = 0; i < count; i++)
            {
                json_object *arg = json_object_array_get_idx(args, i);
                json_object *arg_type = NULL;
                const char *arg_kind = json_string_property(arg, "kind");
                if (json_object_object_get_ex(arg, "type", &arg_type) &&
                    json_string_property_equals(arg_type, "kind", "string") &&
                    arg_kind && (strcmp(arg_kind, "variable") == 0 ||
                                 strcmp(arg_kind, "member") == 0 ||
                                 strcmp(arg_kind, "array_access") == 0))
                    json_object_object_add(arg, "rust_needs_clone",
                                           json_object_new_boolean(true));
            }
        }
    }
}

/* C array search compares non-string elements byte-for-byte. Mark floating
 * searches so Rust preserves C behavior for signed zero and NaN payloads. */
static void rust_lower_array_searches(json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_lower_array_searches(json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_lower_array_searches(value);
    }

    if (!json_string_property_equals(node, "kind", "call")) return;
    json_object *callee = NULL, *object = NULL, *array_type = NULL, *element_type = NULL;
    if (!json_object_object_get_ex(node, "callee", &callee) ||
        !json_string_property_equals(callee, "kind", "member") ||
        !json_object_object_get_ex(callee, "object", &object) ||
        !json_object_object_get_ex(object, "type", &array_type) ||
        !json_string_property_equals(array_type, "kind", "array") ||
        !json_object_object_get_ex(array_type, "element_type", &element_type)) return;

    const char *method = json_string_property(callee, "member_name");
    const char *element_kind = json_string_property(element_type, "kind");
    if (!method || (strcmp(method, "contains") != 0 && strcmp(method, "indexOf") != 0) ||
        !element_kind || (strcmp(element_kind, "float") != 0 &&
                          strcmp(element_kind, "double") != 0)) return;

    json_object_object_add(node, "rust_float_array_search", json_object_new_boolean(true));
    json_object_object_add(node, "rust_float_array_search_type",
                           json_object_new_string(strcmp(element_kind, "float") == 0 ? "f32" : "f64"));
}

static bool rust_owned_value_type(json_object *node)
{
    json_object *type = NULL;
    if (!json_object_object_get_ex(node, "type", &type)) return false;
    const char *kind = json_string_property(type, "kind");
    return kind && (strcmp(kind, "string") == 0 ||
                    strcmp(kind, "array") == 0 ||
                    strcmp(kind, "struct") == 0);
}

static void rust_mark_instance_method_clones(json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_mark_instance_method_clones(json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    const char *kind = json_string_property(node, "kind");
    if (kind && strcmp(kind, "variable") == 0 &&
        json_string_property_equals(node, "name", "self"))
    {
        json_object_object_add(node, "rust_needs_clone",
                               json_object_new_boolean(true));
        return;
    }
    if (kind && strcmp(kind, "member_assign") == 0)
    {
        json_object *value = NULL;
        if (json_object_object_get_ex(node, "value", &value))
            rust_mark_instance_method_clones(value);
        return;
    }
    if (kind && strcmp(kind, "member") == 0)
    {
        json_object *object = NULL;
        if (json_object_object_get_ex(node, "object", &object) &&
            json_string_property_equals(object, "kind", "variable") &&
            json_string_property_equals(object, "name", "self"))
        {
            if (rust_owned_value_type(node))
                json_object_object_add(node, "rust_needs_clone",
                                       json_object_new_boolean(true));
            return;
        }
    }
    if (kind && strcmp(kind, "index_assign") == 0)
    {
        json_object *index = NULL, *value = NULL;
        if (json_object_object_get_ex(node, "index", &index))
            rust_mark_instance_method_clones(index);
        if (!json_boolean_property(node, "source_is_borrow") &&
            json_object_object_get_ex(node, "value", &value))
            rust_mark_instance_method_clones(value);
        return;
    }
    if (kind && strcmp(kind, "call") == 0)
    {
        json_object *args = NULL;
        if (json_object_object_get_ex(node, "args", &args))
            rust_mark_instance_method_clones(args);
        return;
    }

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_mark_instance_method_clones(value);
    }

    if (kind && (strcmp(kind, "member") == 0 ||
                 strcmp(kind, "array_access") == 0) &&
        rust_owned_value_type(node))
        json_object_object_add(node, "rust_needs_clone",
                               json_object_new_boolean(true));
}

static void rust_lower_instance_method_clones(json_object *model)
{
    json_object *structs = NULL;
    if (!json_object_object_get_ex(model, "structs", &structs)) return;
    size_t struct_count = json_object_array_length(structs);
    for (size_t i = 0; i < struct_count; i++)
    {
        json_object *structure = json_object_array_get_idx(structs, i);
        json_object *methods = NULL;
        if (!json_object_object_get_ex(structure, "methods", &methods)) continue;
        size_t method_count = json_object_array_length(methods);
        for (size_t m = 0; m < method_count; m++)
        {
            json_object *method = json_object_array_get_idx(methods, m);
            json_object *body = NULL;
            if (!json_boolean_property(method, "is_static") &&
                json_object_object_get_ex(method, "body", &body))
                rust_mark_instance_method_clones(body);
        }
    }
}

static void rust_lower_interpolation_formats(json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_lower_interpolation_formats(json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_lower_interpolation_formats(value);
    }

    if (!json_string_property_equals(node, "kind", "interpolated_string")) return;
    json_object *parts = NULL;
    if (!json_object_object_get_ex(node, "parts", &parts)) return;
    size_t count = json_object_array_length(parts);
    for (size_t i = 0; i < count; i++)
    {
        json_object *part = json_object_array_get_idx(parts, i);
        json_object *format_spec = NULL, *expr = NULL, *type = NULL;
        if (!json_object_object_get_ex(part, "format_spec", &format_spec) ||
            !json_object_object_get_ex(part, "expr", &expr) ||
            !json_object_object_get_ex(expr, "type", &type)) continue;
        RustFormatSpec parsed;
        char reason[160];
        if (rust_parse_format_spec(json_object_get_string(format_spec),
                                   json_string_property(type, "kind"), &parsed,
                                   reason, sizeof(reason)))
        {
            if (parsed.alternate &&
                (parsed.conversion == 'x' || parsed.conversion == 'X' ||
                 parsed.conversion == 'o'))
            {
                char digits_format[5] = "{:x}";
                digits_format[2] = parsed.conversion;
                json_object_object_add(part, "rust_integer_alternate",
                                       json_object_new_boolean(true));
                json_object_object_add(part, "rust_alternate_digits_format",
                                       json_object_new_string(digits_format));
                json_object_object_add(part, "rust_alternate_uppercase",
                                       json_object_new_boolean(
                                           parsed.conversion == 'X'));
                json_object_object_add(part, "rust_alternate_octal",
                                       json_object_new_boolean(
                                           parsed.conversion == 'o'));
                json_object_object_add(part, "rust_alternate_width",
                                       json_object_new_int(parsed.has_width
                                                           ? parsed.width : 0));
                json_object_object_add(part, "rust_alternate_left_align",
                                       json_object_new_boolean(parsed.left_align));
                json_object_object_add(part, "rust_alternate_zero_pad",
                                       json_object_new_boolean(parsed.zero_pad));
            }
            else if (parsed.alternate && parsed.conversion == 'f')
            {
                json_object_object_add(part, "rust_fixed_alternate",
                                       json_object_new_boolean(true));
                json_object_object_add(part, "rust_fixed_precision",
                                       json_object_new_int(parsed.has_precision
                                                           ? parsed.precision : 6));
                json_object_object_add(part, "rust_fixed_width",
                                       json_object_new_int(parsed.has_width
                                                           ? parsed.width : 0));
                json_object_object_add(part, "rust_fixed_left_align",
                                       json_object_new_boolean(parsed.left_align));
                json_object_object_add(part, "rust_fixed_force_sign",
                                       json_object_new_boolean(parsed.force_sign));
                json_object_object_add(part, "rust_fixed_space_sign",
                                       json_object_new_boolean(parsed.space_sign &&
                                                               !parsed.force_sign));
                json_object_object_add(part, "rust_fixed_zero_pad",
                                       json_object_new_boolean(parsed.zero_pad));
            }
            else if (parsed.conversion == 'c')
            {
                json_object_object_add(part, "rust_character",
                                       json_object_new_boolean(true));
                json_object_object_add(part, "rust_character_width",
                                       json_object_new_int(parsed.has_width
                                                           ? parsed.width : 0));
                json_object_object_add(part, "rust_character_left_align",
                                       json_object_new_boolean(parsed.left_align));
            }
            else if (parsed.conversion == 'e' || parsed.conversion == 'E')
            {
                json_object_object_add(part, "rust_scientific",
                                       json_object_new_boolean(true));
                json_object_object_add(part, "rust_scientific_precision",
                                       json_object_new_int(parsed.has_precision
                                                           ? parsed.precision : 6));
                json_object_object_add(part, "rust_scientific_uppercase",
                                       json_object_new_boolean(parsed.conversion == 'E'));
                json_object_object_add(part, "rust_scientific_width",
                                       json_object_new_int(parsed.has_width
                                                           ? parsed.width : 0));
                json_object_object_add(part, "rust_scientific_left_align",
                                       json_object_new_boolean(parsed.left_align));
                json_object_object_add(part, "rust_scientific_force_sign",
                                       json_object_new_boolean(parsed.force_sign));
                json_object_object_add(part, "rust_scientific_space_sign",
                                       json_object_new_boolean(parsed.space_sign &&
                                                               !parsed.force_sign));
                json_object_object_add(part, "rust_scientific_zero_pad",
                                       json_object_new_boolean(parsed.zero_pad));
                json_object_object_add(part, "rust_scientific_alternate",
                                       json_object_new_boolean(parsed.alternate));
            }
            else
            {
                json_object_object_add(part, "rust_format",
                                       json_object_new_string(parsed.rust_format));
                if (parsed.space_sign && !parsed.force_sign &&
                    (parsed.conversion == 'd' || parsed.conversion == 'i' ||
                     parsed.conversion == 'f'))
                {
                    json_object_object_add(part, "rust_space_sign",
                                           json_object_new_boolean(true));
                    json_object_object_add(part, "rust_space_sign_float",
                                           json_object_new_boolean(
                                               parsed.conversion == 'f'));
                }
            }
            if (parsed.conversion == 's' &&
                (parsed.has_width || parsed.has_precision))
            {
                json_object_object_add(part, "rust_string_format",
                                       json_object_new_boolean(true));
                json_object_object_add(part, "rust_string_width",
                                       json_object_new_int(parsed.has_width
                                                           ? parsed.width : 0));
                json_object_object_add(part, "rust_string_left_align",
                                       json_object_new_boolean(parsed.left_align));
                json_object_object_add(part, "rust_string_has_precision",
                                       json_object_new_boolean(parsed.has_precision));
                json_object_object_add(part, "rust_string_precision",
                                       json_object_new_int(parsed.has_precision
                                                           ? parsed.precision : 0));
            }
        }
    }
}

/* A C-style for loop is rendered as a Rust while loop. Continue statements
 * owned by that loop must therefore run its increment first. Stop at nested
 * loop boundaries because their continue statements have different owners. */
static void rust_mark_for_continues(json_object *node, json_object *increment)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_mark_for_continues(json_object_array_get_idx(node, i), increment);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    const char *kind = json_string_property(node, "kind");
    if (kind && strcmp(kind, "continue") == 0)
    {
        json_object_object_add(node, "rust_continue_increment",
                               json_object_get(increment));
        return;
    }
    if (kind && (strcmp(kind, "for") == 0 || strcmp(kind, "for_each") == 0 ||
                 strcmp(kind, "for_each_iter") == 0 ||
                 strcmp(kind, "while") == 0)) return;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_mark_for_continues(value, increment);
    }
}

static void rust_lower_for_continues(json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_lower_for_continues(json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_lower_for_continues(value);
    }

    if (!json_string_property_equals(node, "kind", "for")) return;
    json_object *body = NULL, *increment = NULL;
    if (json_object_object_get_ex(node, "body", &body) &&
        json_object_object_get_ex(node, "increment", &increment))
        rust_mark_for_continues(body, increment);
}

static bool rust_model_uses_string_helpers(json_object *node)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (rust_model_uses_string_helpers(json_object_array_get_idx(node, i))) return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;
    const char *method = json_string_property(node, "rust_string_method");
    if (method && (strcmp(method, "substring") == 0 || strcmp(method, "replace") == 0 ||
                   strcmp(method, "charAt") == 0 || strcmp(method, "indexOf") == 0))
        return true;
    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (rust_model_uses_string_helpers(value)) return true;
    }
    return false;
}

static bool rust_model_uses_string_format_helpers(json_object *node)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (rust_model_uses_string_format_helpers(
                    json_object_array_get_idx(node, i))) return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;
    json_object *string_width = NULL;
    if (json_object_object_get_ex(node, "rust_string_format", &string_width) ||
        json_object_object_get_ex(node, "rust_character", &string_width) ||
        json_object_object_get_ex(node, "rust_fixed_alternate", &string_width) ||
        json_object_object_get_ex(node, "rust_integer_alternate", &string_width) ||
        json_object_object_get_ex(node, "rust_scientific", &string_width)) return true;
    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (rust_model_uses_string_format_helpers(value)) return true;
    }
    return false;
}

static void rust_mark_scalar_ref_uses(json_object *node, const char *param_name)
{
    if (!node || !param_name) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_mark_scalar_ref_uses(json_object_array_get_idx(node, i), param_name);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    const char *kind = json_string_property(node, "kind");
    if (kind && strcmp(kind, "variable") == 0 &&
        json_boolean_property(node, "is_captured") &&
        !json_boolean_property(node, "is_ref_arg") &&
        json_string_property_equals(node, "name", param_name))
    {
        json_object_object_add(node, "rust_deref", json_object_new_boolean(true));
        return;
    }
    if (kind && strcmp(kind, "assign") == 0 &&
        json_boolean_property(node, "is_captured") &&
        json_string_property_equals(node, "target", param_name))
        json_object_object_add(node, "rust_deref_target", json_object_new_boolean(true));

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_mark_scalar_ref_uses(value, param_name);
    }
}

static void rust_lower_scalar_ref_parameters(json_object *model)
{
    json_object *functions = NULL;
    if (json_object_object_get_ex(model, "functions", &functions))
    {
        size_t function_count = json_object_array_length(functions);
        for (size_t i = 0; i < function_count; i++)
        {
            json_object *function = json_object_array_get_idx(functions, i);
            json_object *params = NULL, *body = NULL;
            if (!json_object_object_get_ex(function, "params", &params) ||
                !json_object_object_get_ex(function, "body", &body)) continue;
            size_t param_count = json_object_array_length(params);
            for (size_t p = 0; p < param_count; p++)
            {
                json_object *param = json_object_array_get_idx(params, p);
                json_object *type = NULL;
                const char *name = json_string_property(param, "name");
                if (name && json_string_property_equals(param, "mem_qual", "as_ref") &&
                    json_object_object_get_ex(param, "type", &type) &&
                    rust_scalar_ref_parameter_type_supported(type))
                    rust_mark_scalar_ref_uses(body, name);
            }
        }
    }

    json_object *structs = NULL;
    if (!json_object_object_get_ex(model, "structs", &structs)) return;
    size_t struct_count = json_object_array_length(structs);
    for (size_t i = 0; i < struct_count; i++)
    {
        json_object *structure = json_object_array_get_idx(structs, i);
        json_object *methods = NULL;
        if (!json_object_object_get_ex(structure, "methods", &methods)) continue;
        size_t method_count = json_object_array_length(methods);
        for (size_t m = 0; m < method_count; m++)
        {
            json_object *method = json_object_array_get_idx(methods, m);
            json_object *params = NULL, *body = NULL;
            if (!json_object_object_get_ex(method, "params", &params) ||
                !json_object_object_get_ex(method, "body", &body)) continue;
            size_t param_count = json_object_array_length(params);
            for (size_t p = 0; p < param_count; p++)
            {
                json_object *param = json_object_array_get_idx(params, p);
                json_object *type = NULL;
                const char *name = json_string_property(param, "name");
                if (name && json_string_property_equals(param, "mem_qual", "as_ref") &&
                    json_object_object_get_ex(param, "type", &type) &&
                    rust_scalar_ref_parameter_type_supported(type))
                    rust_mark_scalar_ref_uses(body, name);
            }
        }
    }
}

/* Rust locals introduced by statement templates share a lexical namespace
 * with Sindarin locals referenced by the rendered body.  Candidate spellings
 * use only [A-Za-z_][A-Za-z0-9_]*, which is also legal in Sindarin, so select
 * each iterator temporary against every string in the complete model.  The full
 * model scan is deliberately schema-independent: it covers declarations,
 * references, loop bindings, and names nested in expressions, while previously
 * assigned temporary names make nested and adjacent iterator loops distinct. */
static bool rust_model_contains_string(json_object *node, const char *wanted)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_string))
        return strcmp(json_object_get_string(node), wanted) == 0;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (rust_model_contains_string(json_object_array_get_idx(node, i), wanted))
                return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (rust_model_contains_string(value, wanted)) return true;
    }
    return false;
}

static bool rust_lower_iterator_temp_names(json_object *model, json_object *node,
                                           size_t *next_id)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (!rust_lower_iterator_temp_names(
                    model, json_object_array_get_idx(node, i), next_id))
                return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (!rust_lower_iterator_temp_names(model, value, next_id)) return false;
    }

    if (!json_string_property_equals(node, "kind", "for_each_iter")) return true;

    char candidate[64];
    do
    {
        if (*next_id == (size_t)-1) return false;
        int written = snprintf(candidate, sizeof(candidate), "__sn_iter_%zu", *next_id);
        (*next_id)++;
        if (written < 0 || (size_t)written >= sizeof(candidate)) return false;
    }
    while (rust_model_contains_string(model, candidate));

    json_object_object_add(node, "rust_iterator_temp_name",
                           json_object_new_string(candidate));
    return true;
}

static bool rust_emit(CompilerOptions *options, Module *module,
                      TargetEmitMode mode, GeneratedFileSet *result)
{
    (void)mode;
    json_object *model = gen_model_build(&options->arena, module,
                                          &options->symbol_table,
                                          options->arithmetic_mode);
    if (!model) return false;
    if (!rust_validate_model(model))
    {
        json_object_put(model);
        return false;
    }
    rust_lower_checked_arithmetic(model);
    rust_lower_checked_mutations(model);
    rust_lower_floating_mutations(model);
    rust_lower_strings(model);
    rust_lower_array_searches(model);
    rust_lower_instance_method_clones(model);
    rust_lower_interpolation_formats(model);
    rust_lower_for_continues(model);
    rust_lower_scalar_ref_parameters(model);
    size_t iterator_temp_id = 0;
    if (!rust_lower_iterator_temp_names(model, model, &iterator_temp_id))
    {
        fprintf(stderr, "Error: Rust target could not assign hygienic iterator temporary names\n");
        json_object_put(model);
        return false;
    }
    if (rust_model_uses_arrays(model))
        json_object_object_add(model, "rust_uses_arrays", json_object_new_boolean(true));
    if (rust_model_uses_reflection(model))
        json_object_object_add(model, "rust_uses_reflection", json_object_new_boolean(true));
    if (rust_model_uses_string_helpers(model))
        json_object_object_add(model, "rust_uses_string_helpers", json_object_new_boolean(true));
    if (rust_model_uses_string_format_helpers(model))
        json_object_object_add(model, "rust_uses_string_format_helpers",
                               json_object_new_boolean(true));

    char template_dir[1024];
    snprintf(template_dir, sizeof(template_dir), "%s/templates/rust", options->compiler_dir);
    char *code = rust_render_model(model, template_dir);
    json_object_put(model);
    if (!code) return false;
    if (!generated_file_set_add(result, "main.rs", code, GENERATED_SOURCE, true))
    {
        free(code);
        return false;
    }
    return true;
}

static bool rust_build(const CompilerOptions *options, const char *build_dir,
                       const GeneratedFileSet *files)
{
    if (files->primary_file < 0) return false;
    char source_path[PATH_MAX];
    snprintf(source_path, sizeof(source_path), "%s/%s", build_dir,
             files->files[files->primary_file].relative_path);

    const char *rustflags = getenv("SN_RUSTFLAGS");
    if (!rustflags) rustflags = "";
    const char *profile_flags = options->debug_build
        ? "-C debuginfo=2 -C opt-level=0"
        : options->profile_build
            ? "-C debuginfo=1 -C opt-level=3 -C force-frame-pointers=yes"
            : "-C opt-level=3";

    char command[PATH_MAX * 3];
    char quoted_rustc[PATH_MAX + 8];
    if (!rustc_quoted(quoted_rustc, sizeof(quoted_rustc)))
    {
        fprintf(stderr,
                "Error: SN_RUSTC path is too long to shell-quote safely; use --emit-rust.\n");
        return false;
    }
    int written = snprintf(command, sizeof(command), "%s --edition=2021 %s %s \"%s\" -o \"%s\"",
                           quoted_rustc, profile_flags, rustflags, source_path,
                           options->executable_file);
    if (written < 0 || (size_t)written >= sizeof(command))
    {
        fprintf(stderr, "Error: failed to build rustc build command\n");
        return false;
    }
    if (options->verbose) DEBUG_INFO("Executing: %s", command);
    if (!rust_run_command(command))
    {
        fprintf(stderr, "Error: rustc failed to build generated source\n");
        return false;
    }
    return true;
}

const TargetCompiler sn_rust_target = {
    TARGET_RUST,
    "rust",
    ".rs",
    "rust",
    rust_check_toolchain,
    rust_emit,
    rust_build
};
