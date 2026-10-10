/* C renders a scalar compound assignment without parentheses. Arithmetic and
 * shifts assign the raw comparison's boolean result. Bitwise operators instead
 * combine the old value with that boolean because their precedence is lower.
 * Checked < and > are helper
 * calls and keep their argument boundary. Keep this compatibility in Rust's
 * private projection; the source, frontend and C renderer remain unchanged. */
static bool rust_lower_compound_comparisons(json_object *model, json_object *node)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            if (!rust_lower_compound_comparisons(model,
                    json_object_array_get_idx(node, i))) return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0 &&
            !rust_lower_compound_comparisons(model, child)) return false;

    if (!json_string_property_equals(node, "kind", "binary")) return true;
    const char *op = json_string_property(node, "op");
    bool strict = op && (strcmp(op, "lt") == 0 || strcmp(op, "gt") == 0);
    bool raw = op && (strcmp(op, "eq") == 0 || strcmp(op, "neq") == 0 ||
                     strcmp(op, "lte") == 0 || strcmp(op, "gte") == 0 ||
                     (strict && !json_string_property_equals(node,
                                      "arithmetic_mode", "checked")));
    if (!raw) return true;
    json_object *left = NULL, *right = NULL, *target = NULL, *value = NULL;
    json_object *target_type = NULL, *value_type = NULL;
    json_object_object_get_ex(node, "left", &left);
    json_object_object_get_ex(node, "right", &right);
    if (!json_string_property_equals(left, "kind", "compound_assign") ||
        json_boolean_property(left, "rust_c_compound_isolated")) return true;
    json_object_object_get_ex(left, "target", &target);
    json_object_object_get_ex(left, "value", &value);
    json_object_object_get_ex(target, "type", &target_type);
    json_object_object_get_ex(value, "type", &value_type);
    const char *target_kind = json_string_property(target_type, "kind");
    const char *rhs_kind = rust_numeric_c_kind(value);
    const char *right_kind = rust_numeric_c_kind(right);
    if (!rhs_kind || !right_kind) return true;
    bool floating = rust_float_arithmetic_pair(target_type, value_type);
    const char *compound_op = json_string_property(left, "op");
    const char *method = NULL;
    bool bitwise = false, shift = false;
    if (!floating)
    {
        if (!rust_fixed_integral_kind(target_kind) ||
            !rust_fixed_integral_kind(rhs_kind) || !compound_op) return true;
        if (strcmp(compound_op, "add") == 0) method = "wrapping_add";
        else if (strcmp(compound_op, "subtract") == 0) method = "wrapping_sub";
        else if (strcmp(compound_op, "multiply") == 0) method = "wrapping_mul";
        else if (strcmp(compound_op, "divide") == 0) method = "wrapping_div";
        else if (strcmp(compound_op, "modulo") == 0) method = "wrapping_rem";
        else if (strcmp(compound_op, "shl") == 0)
        { method = "wrapping_shl"; shift = true; }
        else if (strcmp(compound_op, "shr") == 0)
        { method = "wrapping_shr"; shift = true; }
        else if (strcmp(compound_op, "bitand") == 0 ||
                 strcmp(compound_op, "bitor") == 0 ||
                 strcmp(compound_op, "bitxor") == 0) bitwise = true;
        else return true;
    }
    const char *arithmetic = floating ?
        (strcmp(target_kind, "double") == 0 || strcmp(rhs_kind, "double") == 0 ? "f64" : "f32") :
        rust_integral_promotion_type(target_kind, bitwise ? "int32" : rhs_kind, compound_op);
    const char *arithmetic_kind = floating ?
        (strcmp(arithmetic, "f64") == 0 ? "double" : "float") :
        (strcmp(arithmetic, "u64") == 0 ? "uint" :
         strcmp(arithmetic, "i64") == 0 ? "int" :
         strcmp(arithmetic, "u32") == 0 ? "uint32" : "int32");
    const char *comparison_left = bitwise ? rhs_kind : arithmetic_kind;
    const char *comparison = rust_numeric_floating_kind(comparison_left) || rust_numeric_floating_kind(right_kind) ?
        (strcmp(comparison_left, "double") == 0 || strcmp(right_kind, "double") == 0 ? "f64" : "f32") :
        rust_integral_promotion_type(comparison_left, right_kind, op);
    json_object *rhs = NULL, *other = NULL;
    if (json_object_deep_copy(value, &rhs, NULL) != 0 ||
        json_object_deep_copy(right, &other, NULL) != 0)
    {
        json_object_put(rhs);
        json_object_put(other);
        return false;
    }
    rust_float_array_c_values(rhs);
    rust_numeric_c_values(rhs);
    rust_float_array_c_values(other);
    rust_numeric_c_values(other);
    json_object_object_add(node, "rust_compound_compare_rhs", rhs);
    json_object_object_add(node, "rust_compound_compare_other", other);
    json_object_object_add(node, "rust_compound_compare_arithmetic", json_object_new_string(arithmetic));
    json_object_object_add(node, "rust_compound_compare_common", json_object_new_string(comparison));
    json_object_object_add(node, "rust_compound_compare_rhs_type",
        json_object_new_string(shift ? "u32" : arithmetic));
    json_object_object_add(node, "rust_compound_compare_bitwise", json_object_new_boolean(bitwise));
    if (method)
        json_object_object_add(node, "rust_compound_compare_method", json_object_new_string(method));
    json_object_object_add(node, "rust_compound_compare_storage",
        json_object_new_string(rust_numeric_type_name(target_kind)));
    const char *bases[] = {"__sn_compound_compare_old", "__sn_compound_compare_rhs",
        "__sn_compound_compare_other", "__sn_compound_compare_result", "__sn_compound_compare_next",
        "__sn_compound_compare_place"};
    const char *keys[] = {"rust_compound_compare_old_name", "rust_compound_compare_rhs_name",
        "rust_compound_compare_other_name", "rust_compound_compare_result_name", "rust_compound_compare_next_name",
        "rust_compound_compare_place_name"};
    for (size_t i = 0; i < 6; i++)
    {
        char name[96];
        if (!rust_allocate_helper_name(model, bases[i], name, sizeof(name))) return false;
        json_object_object_add(node, keys[i], json_object_new_string(name));
    }
    json_object_object_add(node, "rust_c_compound_comparison", json_object_new_boolean(true));
    return true;
}
