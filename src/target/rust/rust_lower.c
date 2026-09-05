/* Included after validation; uses the same private type/model helpers. */
#include "rust_lower_calls.c"
#include "rust_lower_closures.c"

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
    const char *method = NULL, *error_name = NULL;
    if (strcmp(op, "add") == 0) { method = "checked_add"; error_name = "addition"; }
    else if (strcmp(op, "subtract") == 0) { method = "checked_sub"; error_name = "subtraction"; }
    else if (strcmp(op, "multiply") == 0) { method = "checked_mul"; error_name = "multiplication"; }
    else if (strcmp(op, "divide") == 0) { method = "checked_div"; error_name = "division"; }
    else if (strcmp(op, "modulo") == 0) { method = "checked_rem"; error_name = "modulo"; }
    if (method)
    {
        json_object_object_add(node, "rust_checked_method", json_object_new_string(method));
        json_object_object_add(node, "rust_checked_operation", json_object_new_string(op));
        json_object_object_add(node, "rust_checked_error_name", json_object_new_string(error_name));
    }
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

    const char *method = NULL, *error_name = NULL;
    if (strcmp(op, "add") == 0) { method = "checked_add"; error_name = "addition"; }
    else if (strcmp(op, "subtract") == 0) { method = "checked_sub"; error_name = "subtraction"; }
    else if (strcmp(op, "multiply") == 0) { method = "checked_mul"; error_name = "multiplication"; }
    else if (strcmp(op, "divide") == 0) { method = "checked_div"; error_name = "division"; }
    else if (strcmp(op, "modulo") == 0) { method = "checked_rem"; error_name = "modulo"; }
    if (method)
    {
        json_object_object_add(node, "rust_checked_method", json_object_new_string(method));
        json_object_object_add(node, "rust_checked_operation", json_object_new_string(op));
        json_object_object_add(node, "rust_checked_error_name", json_object_new_string(error_name));
    }
}

/* This is deliberately a Rust-rendering concern: C retains its own established
 * runtime implementation.  The marker lets the module include one shared Rust
 * diagnostic helper only for generated programs that use checked arithmetic. */
static bool rust_model_uses_checked_arithmetic(json_object *node)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (rust_model_uses_checked_arithmetic(json_object_array_get_idx(node, i)))
                return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;
    json_object *method = NULL;
    if (json_object_object_get_ex(node, "rust_checked_method", &method)) return true;
    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (rust_model_uses_checked_arithmetic(value)) return true;
    }
    return false;
}

static bool rust_assign_checked_helper_names(json_object *model)
{
    const char *bases[] = { "__sn_checked", "__sn_checked_div", "__sn_checked_mod",
                            "__sn_runtime_error" };
    const char *keys[] = { "rust_checked_name", "rust_checked_div_name",
                           "rust_checked_mod_name", "rust_runtime_error_name" };

    for (size_t i = 0; i < 4; i++)
    {
        char name[96];
        size_t suffix = 0;
        while (true)
        {
            int written = snprintf(name, sizeof(name), "%s_%zu", bases[i], suffix);
            if (written < 0 || (size_t)written >= sizeof(name)) return false;
            if (!rust_model_contains_string(model, name)) break;
            if (suffix == (size_t)-1) return false;
            suffix++;
        }
        json_object_object_add(model, keys[i], json_object_new_string(name));
    }
    return true;
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

    /* Result ownership is independent of the match subject family. A str
     * result is itself owned, so direct reads through live owners need one
     * clone whether the subject is bool, numeric, or str. Literals already
     * allocate a String, while concat/interpolation/calls/nested matches
     * already produce owned values. */
    if (strcmp(kind, "match") == 0 &&
        json_boolean_property(node, "rust_value_match"))
    {
        json_object *type = NULL, *arms = NULL;
        if (json_object_object_get_ex(node, "type", &type) &&
            json_string_property_equals(type, "kind", "string") &&
            json_object_object_get_ex(node, "arms", &arms))
        {
            size_t arm_count = json_object_array_length(arms);
            for (size_t i = 0; i < arm_count; i++)
            {
                json_object *arm = json_object_array_get_idx(arms, i);
                json_object *body = NULL, *statements = NULL;
                if (!json_object_object_get_ex(arm, "body", &body) ||
                    !json_object_object_get_ex(body, "statements", &statements) ||
                    json_object_array_length(statements) == 0)
                    continue;
                size_t statement_count = json_object_array_length(statements);
                json_object *statement =
                    json_object_array_get_idx(statements, statement_count - 1);
                json_object *result = NULL;
                if (!json_object_object_get_ex(statement, "expr", &result))
                    continue;
                const char *result_kind = json_string_property(result, "kind");
                if (result_kind &&
                    (strcmp(result_kind, "variable") == 0 ||
                     strcmp(result_kind, "member") == 0 ||
                     strcmp(result_kind, "array_access") == 0))
                    json_object_object_add(result, "rust_needs_clone",
                                           json_object_new_boolean(true));
            }
        }
    }
    if (strcmp(kind, "match") == 0 &&
        json_boolean_property(node, "rust_string_match"))
    {
        json_object *subject = NULL;
        if (json_object_object_get_ex(node, "subject", &subject))
        {
            const char *subject_kind = json_string_property(subject, "kind");
            if (subject_kind &&
                (strcmp(subject_kind, "variable") == 0 ||
                 strcmp(subject_kind, "member") == 0))
                json_object_object_add(subject, "rust_needs_clone",
                                       json_object_new_boolean(true));
        }

        return;
    }
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
    rust_lower_call_strings(node, kind);
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

/* Member assignment evaluates its value before its destination place.  When
 * that place contains an indexed projection, rendering the projection twice
 * (once for len() and once for indexing) both duplicates work and asks Rust
 * for overlapping immutable/mutable borrows.  Record each raw index in
 * root-to-leaf order, then let the place renderer normalize it while holding
 * one short mutable borrow of that projection's owner. */
static bool rust_collect_member_place_indices(json_object *model,
                                              json_object *place,
                                              json_object *bindings,
                                              size_t *next_id)
{
    if (!place || !json_object_is_type(place, json_type_object)) return true;

    const char *kind = json_string_property(place, "kind");
    if (!kind) return true;

    if (strcmp(kind, "member") == 0 || strcmp(kind, "member_access") == 0)
    {
        json_object *object = NULL;
        if (json_object_object_get_ex(place, "object", &object))
            return rust_collect_member_place_indices(model, object, bindings,
                                                     next_id);
        return true;
    }

    if (strcmp(kind, "array_access") != 0) return true;

    json_object *array = NULL, *index = NULL;
    if (!json_object_object_get_ex(place, "array", &array) ||
        !json_object_object_get_ex(place, "index", &index))
        return false;
    if (!rust_collect_member_place_indices(model, array, bindings, next_id))
        return false;

    char raw_name[80], owner_name[80], index_name[80];
    do
    {
        if (*next_id == (size_t)-1) return false;
        size_t id = *next_id;
        (*next_id)++;
        int raw_written = snprintf(raw_name, sizeof(raw_name),
                                   "__sn_place_raw_index_%zu", id);
        int owner_written = snprintf(owner_name, sizeof(owner_name),
                                     "__sn_place_owner_%zu", id);
        int index_written = snprintf(index_name, sizeof(index_name),
                                     "__sn_place_index_%zu", id);
        if (raw_written < 0 || (size_t)raw_written >= sizeof(raw_name) ||
            owner_written < 0 || (size_t)owner_written >= sizeof(owner_name) ||
            index_written < 0 || (size_t)index_written >= sizeof(index_name))
            return false;
    }
    while (rust_model_contains_string(model, raw_name) ||
           rust_model_contains_string(model, owner_name) ||
           rust_model_contains_string(model, index_name));

    json_object_object_add(place, "rust_place_raw_index_name",
                           json_object_new_string(raw_name));
    json_object_object_add(place, "rust_place_owner_name",
                           json_object_new_string(owner_name));
    json_object_object_add(place, "rust_place_index_name",
                           json_object_new_string(index_name));

    json_object *binding = json_object_new_object();
    if (!binding) return false;
    json_object_object_add(binding, "rust_place_raw_index_name",
                           json_object_new_string(raw_name));
    json_object_object_add(binding, "index", json_object_get(index));
    json_object_array_add(bindings, binding);
    return true;
}

static bool rust_lower_member_assignment_places(json_object *model,
                                                json_object *node,
                                                size_t *next_id)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (!rust_lower_member_assignment_places(
                    model, json_object_array_get_idx(node, i), next_id))
                return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (!rust_lower_member_assignment_places(model, value, next_id))
            return false;
    }

    if (!json_string_property_equals(node, "kind", "member_assign")) return true;
    json_object *object = NULL;
    if (!json_object_object_get_ex(node, "object", &object)) return false;

    json_object *bindings = json_object_new_array();
    if (!bindings) return false;
    if (!rust_collect_member_place_indices(model, object, bindings, next_id))
    {
        json_object_put(bindings);
        return false;
    }
    if (json_object_array_length(bindings) == 0)
    {
        json_object_put(bindings);
        return true;
    }

    char value_name[80];
    do
    {
        if (*next_id == (size_t)-1)
        {
            json_object_put(bindings);
            return false;
        }
        size_t id = *next_id;
        (*next_id)++;
        int written = snprintf(value_name, sizeof(value_name),
                               "__sn_place_value_%zu", id);
        if (written < 0 || (size_t)written >= sizeof(value_name))
        {
            json_object_put(bindings);
            return false;
        }
    }
    while (rust_model_contains_string(model, value_name));

    json_object_object_add(node, "rust_place_value_name",
                           json_object_new_string(value_name));
    json_object_object_add(node, "rust_place_index_bindings", bindings);
    return true;
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

/* Lowered match templates introduce locals that remain in scope while arm
 * bodies render.  Assign every such match its own spellings, absent from
 * the complete model, so source declarations and references cannot be
 * captured by those generated bindings.  The schema-independent string scan
 * also sees names already assigned to nested matches. */
static bool rust_lower_match_temp_names(json_object *model, json_object *node,
                                        size_t *next_id)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (!rust_lower_match_temp_names(
                    model, json_object_array_get_idx(node, i), next_id))
                return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (!rust_lower_match_temp_names(model, value, next_id)) return false;
    }

    if (!json_string_property_equals(node, "kind", "match") ||
        (!json_boolean_property(node, "rust_string_match") &&
         !json_boolean_property(node, "rust_floating_match")))
        return true;

    char subject_name[80], array_name[80], index_name[80];
    do
    {
        if (*next_id == (size_t)-1) return false;
        size_t id = *next_id;
        (*next_id)++;
        int subject_written = snprintf(subject_name, sizeof(subject_name),
                                       "__sn_match_subject_%zu", id);
        int array_written = snprintf(array_name, sizeof(array_name),
                                     "__sn_match_array_%zu", id);
        int index_written = snprintf(index_name, sizeof(index_name),
                                     "__sn_match_index_%zu", id);
        if (subject_written < 0 || (size_t)subject_written >= sizeof(subject_name) ||
            array_written < 0 || (size_t)array_written >= sizeof(array_name) ||
            index_written < 0 || (size_t)index_written >= sizeof(index_name))
            return false;
    }
    while (rust_model_contains_string(model, subject_name) ||
           rust_model_contains_string(model, array_name) ||
           rust_model_contains_string(model, index_name));

    json_object_object_add(node, "rust_match_subject_name",
                           json_object_new_string(subject_name));
    json_object_object_add(node, "rust_match_array_name",
                           json_object_new_string(array_name));
    json_object_object_add(node, "rust_match_index_name",
                           json_object_new_string(index_name));
    return true;
}
