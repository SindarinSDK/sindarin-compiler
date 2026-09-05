/* Included by rust_target.c: shared validation, recursion and model sequencing. */
#include "rust_validate_internal.h"

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
    if (strcmp(kind, "function") == 0) return rust_closure_type_supported(type);
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

/* Direct assignment is intentionally narrower than Rust's general Copy set.
 * These are exactly the source scalar kinds whose default parameter ABI is a
 * genuine callee-local value in both the C and Rust targets. */
static bool rust_by_value_assign_parameter_type_supported(json_object *type)
{
    return rust_scalar_ref_parameter_type_supported(type);
}

static bool rust_direct_variable_named(json_object *node, const char *name)
{
    return node && name &&
        json_string_property_equals(node, "kind", "variable") &&
        json_string_property_equals(node, "name", name);
}

/* Rust cannot preserve the source sequencing contract if the RHS can mutate
 * the very parameter whose replacement is still pending. Keep this bounded:
 * ordinary reads of the old Copy value are allowed, while a nested write or
 * an as-ref argument of that same parameter is rejected. */
static bool rust_rhs_mutates_or_forwards_parameter(json_object *node,
                                                    const char *name)
{
    if (!node || !name) return false;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (rust_rhs_mutates_or_forwards_parameter(
                    json_object_array_get_idx(node, i), name)) return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;

    /* A lambda has its own bindings and is rejected separately as a closure. */
    if (json_string_property_equals(node, "kind", "lambda")) return false;

    const char *kind = json_string_property(node, "kind");
    if (kind && strcmp(kind, "variable") == 0 &&
        json_boolean_property(node, "is_ref_arg") &&
        json_string_property_equals(node, "name", name)) return true;

    if (kind && strcmp(kind, "assign") == 0 &&
        json_string_property_equals(node, "target", name)) return true;

    json_object *place = NULL;
    if (kind && strcmp(kind, "compound_assign") == 0 &&
        json_object_object_get_ex(node, "target", &place) &&
        rust_direct_variable_named(place, name)) return true;
    if (kind && (strcmp(kind, "increment") == 0 ||
                 strcmp(kind, "decrement") == 0) &&
        json_object_object_get_ex(node, "operand", &place) &&
        rust_direct_variable_named(place, name)) return true;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (rust_rhs_mutates_or_forwards_parameter(value, name)) return true;
    }
    return false;
}

static json_object *rust_find_parameter(json_object *params, const char *name)
{
    if (!params || !name) return NULL;
    size_t count = json_object_array_length(params);
    for (size_t i = 0; i < count; i++)
    {
        json_object *param = json_object_array_get_idx(params, i);
        if (json_string_property_equals(param, "name", name)) return param;
    }
    return NULL;
}

typedef struct RustLocalBindingScope
{
    const char *name;
    struct RustLocalBindingScope *parent;
} RustLocalBindingScope;

static bool rust_name_is_shadowed(RustLocalBindingScope *scope,
                                  const char *name)
{
    for (; scope; scope = scope->parent)
        if (scope->name && name && strcmp(scope->name, name) == 0) return true;
    return false;
}

static json_object *rust_assignment_place_root(json_object *place)
{
    if (!json_object_is_type(place, json_type_object)) return NULL;
    if (json_string_property_equals(place, "kind", "variable")) return place;

    json_object *parent = NULL;
    if (json_string_property_equals(place, "kind", "member") &&
        json_object_object_get_ex(place, "object", &parent))
        return rust_assignment_place_root(parent);
    if (json_string_property_equals(place, "kind", "array_access") &&
        json_object_object_get_ex(place, "array", &parent))
        return rust_assignment_place_root(parent);
    return NULL;
}

static bool rust_prepare_parameter_mutations_in_node(json_object *node,
                                                     json_object *params,
                                                     RustLocalBindingScope *scope)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        RustLocalBindingScope *bindings = count
            ? calloc(count, sizeof(RustLocalBindingScope)) : NULL;
        if (count && !bindings) return false;
        RustLocalBindingScope *current = scope;
        for (size_t i = 0; i < count; i++)
        {
            json_object *element = json_object_array_get_idx(node, i);
            if (!rust_prepare_parameter_mutations_in_node(
                    element, params, current))
            {
                free(bindings);
                return false;
            }
            if (json_string_property_equals(element, "kind", "var_decl"))
            {
                bindings[i].name = json_string_property(element, "name");
                bindings[i].parent = current;
                current = &bindings[i];
            }
        }
        free(bindings);
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;

    /* A lambda owns a distinct parameter/local scope. Rust rejects closures
     * before lowering, so do not apply the enclosing callable's by-value
     * parameter table while preparing its nested body. */
    if (json_string_property_equals(node, "kind", "lambda")) return true;

    if (json_string_property_equals(node, "kind", "var_decl"))
    {
        const char *binding_name = json_string_property(node, "name");
        json_object *param = rust_name_is_shadowed(scope, binding_name)
            ? NULL : rust_find_parameter(params, binding_name);
        json_object *type = NULL, *initializer = NULL;
        if (param &&
            json_string_property_equals(param, "mem_qual", "default") &&
            json_object_object_get_ex(param, "type", &type) &&
            rust_by_value_assign_parameter_type_supported(type) &&
            json_object_object_get_ex(node, "initializer", &initializer) &&
            rust_rhs_mutates_or_forwards_parameter(initializer, binding_name))
        {
            fprintf(stderr,
                    "Error: Rust target does not support a local declaration shadowing by-value parameter '%s' when its initializer mutates or forwards that parameter\n",
                    binding_name);
            return false;
        }
    }

    if (json_string_property_equals(node, "kind", "member_assign") ||
        json_string_property_equals(node, "kind", "index_assign"))
    {
        bool member = json_string_property_equals(node, "kind", "member_assign");
        json_object *place = NULL;
        json_object *root = NULL;
        const char *root_name = NULL;
        if (!json_object_object_get_ex(node, member ? "object" : "array", &place))
            return false;
        root = rust_assignment_place_root(place);
        if (!root)
        {
            fprintf(stderr,
                    "Error: Rust target does not support direct assignment to computed %s targets\n",
                    member ? "field" : "index");
            return false;
        }
        root_name = json_string_property(root, "name");
        json_object *param = rust_name_is_shadowed(scope, root_name)
            ? NULL : rust_find_parameter(params, root_name);
        if (param && json_string_property_equals(param, "mem_qual", "default"))
        {
            fprintf(stderr,
                    "Error: Rust target does not support direct assignment through %s targets rooted in by-value parameter '%s'\n",
                    member ? "field" : "index", root_name);
            return false;
        }
    }

    if (json_string_property_equals(node, "kind", "assign"))
    {
        const char *target = json_string_property(node, "target");
        json_object *param = rust_name_is_shadowed(scope, target)
            ? NULL : rust_find_parameter(params, target);
        if (param && json_string_property_equals(param, "mem_qual", "default"))
        {
            json_object *type = NULL, *value = NULL, *value_type = NULL;
            const char *type_kind = NULL, *value_kind = NULL;
            if (!json_object_object_get_ex(param, "type", &type) ||
                !(type_kind = json_string_property(type, "kind")) ||
                !rust_by_value_assign_parameter_type_supported(type))
            {
                fprintf(stderr,
                        "Error: Rust target does not support direct assignment of by-value parameter '%s' with type '%s'; only bool, int, long, int32, byte, uint32, uint, float, and double are supported\n",
                        target ? target : "<anonymous>",
                        type_kind ? type_kind : "<unknown>");
                return false;
            }
            if (!json_object_object_get_ex(node, "value", &value)) return false;
            if (!json_object_object_get_ex(value, "type", &value_type) ||
                !(value_kind = json_string_property(value_type, "kind")) ||
                strcmp(type_kind, value_kind) != 0)
            {
                fprintf(stderr,
                        "Error: Rust target requires direct assignment of by-value parameter '%s' to use the exact same scalar type; cannot assign '%s' to '%s'\n",
                        target, value_kind ? value_kind : "<unknown>", type_kind);
                return false;
            }
            if (rust_rhs_mutates_or_forwards_parameter(value, target))
            {
                fprintf(stderr,
                        "Error: Rust target does not support direct assignment of by-value parameter '%s' when its RHS mutates or forwards the same parameter as ref\n",
                        target);
                return false;
            }
            json_object_object_add(param, "rust_by_value_mutated",
                                   json_object_new_boolean(true));
            json_object_object_add(node, "rust_by_value_scalar_parameter_assign",
                                   json_object_new_boolean(true));
        }
    }

    if (json_string_property_equals(node, "kind", "compound_assign"))
    {
        json_object *target = NULL, *value = NULL, *param = NULL;
        const char *target_name = NULL;
        if (json_object_object_get_ex(node, "target", &target) &&
            json_string_property_equals(target, "kind", "variable") &&
            json_string_property_equals(node, "mutation_storage", "parameter") &&
            json_string_property_equals(node, "mutation_place", "variable") &&
            (target_name = json_string_property(target, "name")) &&
            !rust_name_is_shadowed(scope, target_name) &&
            (param = rust_find_parameter(params, target_name)) &&
            json_string_property_equals(param, "mem_qual", "default"))
        {
            json_object *param_type = NULL, *value_type = NULL;
            const char *param_kind = NULL, *value_kind = NULL;
            const char *op = json_string_property(node, "op");
            if (json_object_object_get_ex(param, "type", &param_type) &&
                (param_kind = json_string_property(param_type, "kind")) &&
                (strcmp(param_kind, "float") == 0 ||
                 strcmp(param_kind, "double") == 0 ||
                 strcmp(param_kind, "byte") == 0 ||
                 strcmp(param_kind, "uint32") == 0 ||
                 strcmp(param_kind, "uint") == 0))
            {
                bool wrapping_parameter = strcmp(param_kind, "byte") == 0 ||
                    strcmp(param_kind, "uint32") == 0 ||
                    strcmp(param_kind, "uint") == 0;
                if (!op || (strcmp(op, "add") != 0 &&
                            strcmp(op, "subtract") != 0 &&
                            strcmp(op, "multiply") != 0 &&
                            strcmp(op, "divide") != 0 &&
                            (!wrapping_parameter ||
                             (strcmp(op, "modulo") != 0 &&
                              strcmp(op, "bitand") != 0 &&
                              strcmp(op, "bitor") != 0 &&
                              strcmp(op, "bitxor") != 0 &&
                              strcmp(op, "shl") != 0 &&
                              strcmp(op, "shr") != 0))))
                {
                    fprintf(stderr, "%s",
                        wrapping_parameter
                            ? "Error: Rust target supports by-value wrapping-integer compound assignment only for +=, -=, *=, /=, %=, &=, |=, ^=, <<=, and >>=\n"
                            : "Error: Rust target supports floating-point compound assignment only for +=, -=, *=, and /=\n");
                    return false;
                }
                if (!json_object_object_get_ex(node, "value", &value) ||
                    !json_object_object_get_ex(value, "type", &value_type) ||
                    !(value_kind = json_string_property(value_type, "kind")) ||
                    strcmp(param_kind, value_kind) != 0)
                {
                    fprintf(stderr, "%s",
                        wrapping_parameter
                            ? "Error: Rust target requires by-value wrapping-integer compound assignment to use same-type operands\n"
                            : "Error: Rust target currently supports floating-point compound assignment only between same-type float or double operands\n");
                    return false;
                }
                if (rust_rhs_mutates_or_forwards_parameter(value, target_name))
                {
                    fprintf(stderr,
                            "Error: Rust target does not support %s compound assignment of by-value parameter '%s' when its RHS mutates or forwards the same parameter as ref\n",
                            wrapping_parameter ? "wrapping-integer" : "floating-point", target_name);
                    return false;
                }
                json_object_object_add(param, "rust_by_value_mutated",
                                       json_object_new_boolean(true));
                json_object_object_add(node,
                    wrapping_parameter ? "rust_by_value_wrapping_parameter_mutation" :
                                         "rust_by_value_floating_parameter_mutation",
                    json_object_new_boolean(true));
            }
        }
    }

    if (json_string_property_equals(node, "kind", "increment") ||
        json_string_property_equals(node, "kind", "decrement"))
    {
        json_object *operand = NULL, *param = NULL, *param_type = NULL;
        const char *operand_name = NULL, *param_kind = NULL;
        if (json_object_object_get_ex(node, "operand", &operand) &&
            json_string_property_equals(operand, "kind", "variable") &&
            json_string_property_equals(node, "mutation_storage", "parameter") &&
            json_string_property_equals(node, "mutation_place", "variable") &&
            (operand_name = json_string_property(operand, "name")) &&
            !rust_name_is_shadowed(scope, operand_name) &&
            (param = rust_find_parameter(params, operand_name)) &&
            json_string_property_equals(param, "mem_qual", "default") &&
            json_object_object_get_ex(param, "type", &param_type) &&
            (param_kind = json_string_property(param_type, "kind")) &&
            (strcmp(param_kind, "float") == 0 ||
             strcmp(param_kind, "double") == 0 ||
             strcmp(param_kind, "byte") == 0 ||
             strcmp(param_kind, "uint32") == 0 ||
             strcmp(param_kind, "uint") == 0))
        {
            json_object_object_add(param, "rust_by_value_mutated",
                                   json_object_new_boolean(true));
            json_object_object_add(node,
                (strcmp(param_kind, "byte") == 0 ||
                 strcmp(param_kind, "uint32") == 0 ||
                 strcmp(param_kind, "uint") == 0) ?
                    "rust_by_value_wrapping_parameter_mutation" :
                    "rust_by_value_floating_parameter_mutation",
                json_object_new_boolean(true));
        }
    }

    if (json_string_property_equals(node, "kind", "for_each") ||
        json_string_property_equals(node, "kind", "for_each_iter"))
    {
        json_object *iterable = NULL, *body = NULL;
        if (json_object_object_get_ex(node, "iterable", &iterable) &&
            !rust_prepare_parameter_mutations_in_node(iterable, params, scope))
            return false;
        const char *binding_name = json_string_property(node, "iterator_name");
        RustLocalBindingScope binding = {binding_name, scope};
        return !json_object_object_get_ex(node, "body", &body) ||
            rust_prepare_parameter_mutations_in_node(
                body, params, binding_name ? &binding : scope);
    }
    if (json_string_property_equals(node, "kind", "for"))
    {
        json_object *init = NULL, *condition = NULL, *increment = NULL, *body = NULL;
        if (!json_object_object_get_ex(node, "init", &init) ||
            !rust_prepare_parameter_mutations_in_node(init, params, scope))
            return false;
        const char *binding_name = json_string_property(init, "name");
        RustLocalBindingScope binding = {binding_name, scope};
        RustLocalBindingScope *loop_scope = binding_name ? &binding : scope;
        if (json_object_object_get_ex(node, "condition", &condition) &&
            !rust_prepare_parameter_mutations_in_node(
                condition, params, loop_scope)) return false;
        if (json_object_object_get_ex(node, "body", &body) &&
            !rust_prepare_parameter_mutations_in_node(
                body, params, loop_scope)) return false;
        return !json_object_object_get_ex(node, "increment", &increment) ||
            rust_prepare_parameter_mutations_in_node(
                increment, params, loop_scope);
    }

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (!rust_prepare_parameter_mutations_in_node(value, params, scope))
            return false;
    }
    return true;
}

static bool rust_prepare_callable_parameter_mutations(json_object *callable)
{
    json_object *params = NULL, *body = NULL;
    if (!json_object_object_get_ex(callable, "params", &params) ||
        !json_object_object_get_ex(callable, "body", &body)) return true;
    return rust_prepare_parameter_mutations_in_node(body, params, NULL);
}

static bool rust_prepare_by_value_scalar_parameter_mutations(json_object *model)
{
    json_object *functions = NULL;
    if (json_object_object_get_ex(model, "functions", &functions))
    {
        size_t count = json_object_array_length(functions);
        for (size_t i = 0; i < count; i++)
            if (!rust_prepare_callable_parameter_mutations(
                    json_object_array_get_idx(functions, i))) return false;
    }

    json_object *structs = NULL;
    if (!json_object_object_get_ex(model, "structs", &structs)) return true;
    size_t struct_count = json_object_array_length(structs);
    for (size_t i = 0; i < struct_count; i++)
    {
        json_object *methods = NULL;
        json_object *structure = json_object_array_get_idx(structs, i);
        if (!json_object_object_get_ex(structure, "methods", &methods)) continue;
        size_t method_count = json_object_array_length(methods);
        for (size_t m = 0; m < method_count; m++)
            if (!rust_prepare_callable_parameter_mutations(
                    json_object_array_get_idx(methods, m))) return false;
    }
    return true;
}

static bool rust_floating_type(json_object *type)
{
    return json_string_property_equals(type, "kind", "float") ||
        json_string_property_equals(type, "kind", "double");
}

static bool rust_floating_ref_parameter(json_object *mutation,
                                        json_object *parameter)
{
    json_object *type = NULL;
    return json_string_property_equals(mutation, "mutation_storage", "parameter") &&
        json_string_property_equals(mutation, "mutation_place", "variable") &&
        json_string_property_equals(parameter, "kind", "variable") &&
        json_string_property_equals(parameter, "parameter_mem_qual", "as_ref") &&
        json_object_object_get_ex(parameter, "type", &type) &&
        rust_floating_type(type);
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
static ArithmeticMode rust_validation_arithmetic_mode;

typedef struct RustIteratorBindingScope
{
    const char *name;
    struct RustIteratorBindingScope *parent;
} RustIteratorBindingScope;

static RustIteratorBindingScope *rust_iterator_binding_scope;

static bool rust_validate_expr(json_object *expr);
static bool rust_validate_value_match(json_object *expr);
static bool rust_integer_type(const char *kind);
static bool rust_float_type(const char *kind);
static bool rust_is_mutating_array_call(json_object *node);

/* Iterator-protocol bindings are represented as parameters by the shared
 * model, even though the Rust template creates a fresh mutable value from each
 * next() result.  Mark only a direct mutation of an active binding here so the
 * Rust validator can distinguish it from a genuine by-value parameter without
 * changing the shared model.  The scope chain preserves outer bindings across
 * nested loops and naturally gives repeated binding names innermost scope. */
static bool rust_mark_iterator_binding_mutation(json_object *mutation,
                                                json_object *target)
{
    const char *name = json_string_property(target, "name");
    if (!name ||
        !json_string_property_equals(mutation, "mutation_storage", "parameter") ||
        !json_string_property_equals(mutation, "mutation_place", "variable") ||
        !json_string_property_equals(target, "kind", "variable") ||
        !json_boolean_property(target, "is_parameter") ||
        !json_string_property_equals(target, "parameter_mem_qual", "default"))
        return false;

    for (RustIteratorBindingScope *scope = rust_iterator_binding_scope;
         scope; scope = scope->parent)
    {
        if (strcmp(scope->name, name) == 0)
        {
            json_object_object_add(mutation, "rust_iterator_binding_mutation",
                                   json_object_new_boolean(true));
            /* The shared storage classification also selects unchecked mode
             * for parameter mutations.  This binding is a fresh Rust local,
             * so restore the ordinary checked-local mode for integral
             * compound and postfix operations before existing validation and
             * lowering inspect it. */
            json_object *type = NULL;
            const char *type_kind = NULL;
            if (rust_validation_arithmetic_mode == ARITH_CHECKED &&
                json_object_object_get_ex(target, "type", &type) &&
                (type_kind = json_string_property(type, "kind")) &&
                rust_integer_type(type_kind))
                json_object_object_add(mutation, "mutation_arithmetic_mode",
                                       json_object_new_string("checked"));
            return true;
        }
    }
    return false;
}

static bool rust_report_match_error(const char *message)
{
    rust_validation_reported_error = true;
    fprintf(stderr, "Error: Rust target %s\n", message);
    return false;
}

/* Keep this list aligned with the source-language primitive conversion members.
 * Unsupported source-valid conversions receive a target-specific diagnostic below;
 * model-only names such as toFloat/toBool deliberately are not recognized here. */
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
    if (json_string_property_equals(type, "kind", "function"))
        return rust_closure_type_supported(type);
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

#include "rust_validate_closures.c"
#include "rust_validate_calls.c"

static bool rust_validate_expr(json_object *expr)
{
    json_object *kind_obj = NULL;
    if (!expr || !json_object_object_get_ex(expr, "kind", &kind_obj)) return false;
    const char *kind = json_object_get_string(kind_obj);
    json_object *child = NULL;
    if (!kind) return false;

    if (json_boolean_property(expr, "rust_shared_cell") &&
        !json_boolean_property(expr, "rust_shared_owned_cell") &&
        (strcmp(kind, "compound_assign") == 0 || strcmp(kind, "increment") == 0 ||
         strcmp(kind, "decrement") == 0))
        return rust_validate_closure_cell_mutation(expr);
    if (json_boolean_property(expr, "rust_snapshot_mutation"))
        return rust_validate_closure_snapshot_mutation(expr);

    if (strcmp(kind, "match") == 0)
        return rust_validate_value_match(expr);

    if (strcmp(kind, "literal") == 0)
    {
        json_object *reflected_type = NULL;
        if (json_object_object_get_ex(expr, "reflected_type", &reflected_type))
            return rust_validate_typeof_operand(expr, reflected_type);
        return true;
    }
    if (strcmp(kind, "variable") == 0) return rust_validate_function_value(expr);
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

        if (strcmp(operand_kind, "function") == 0)
            return rust_closure_type_supported(operand_type) && rust_validate_expr(operand);

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
        return rust_validate_static_call(expr);
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
        bool iterator_binding_mutation =
            rust_mark_iterator_binding_mutation(expr, target);
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
            if (json_string_property_equals(
                    expr, "mutation_storage", "parameter") &&
                !json_string_property_equals(
                    target, "parameter_mem_qual", "as_ref") &&
                !json_boolean_property(expr, "rust_shared_owned_cell") &&
                !iterator_binding_mutation)
            {
                fprintf(stderr,
                        "Error: Rust target does not support compound assignment of by-value parameters\n");
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
            if (json_string_property_equals(
                    expr, "mutation_storage", "parameter") &&
                !json_string_property_equals(
                    target, "parameter_mem_qual", "as_ref") &&
                !json_boolean_property(
                    expr, "rust_by_value_floating_parameter_mutation") &&
                !iterator_binding_mutation)
            {
                fprintf(stderr,
                        "Error: Rust target does not support compound assignment of by-value parameters\n");
                return false;
            }
            if (!json_string_property_equals(expr, "mutation_storage", "local") &&
                !rust_floating_ref_parameter(expr, target) &&
                !json_boolean_property(
                    expr, "rust_by_value_floating_parameter_mutation") &&
                !iterator_binding_mutation)
            {
                fprintf(stderr,
                        "Error: Rust target supports floating-point compound assignment only for stable mutable locals and direct fields\n");
                return false;
            }
            return rust_validate_expr(target) && rust_validate_expr(value);
        }
        if (json_string_property_equals(expr, "mutation_storage", "parameter") &&
            !json_string_property_equals(target, "parameter_mem_qual", "as_ref") &&
            !json_boolean_property(
                expr, "rust_by_value_wrapping_parameter_mutation") &&
            !iterator_binding_mutation)
        {
            fprintf(stderr,
                    "Error: Rust target does not support compound assignment of by-value parameters\n");
            return false;
        }
        bool checked_ref_parameter = rust_checked_scalar_ref_parameter(expr, target);
        bool mixed_integer =
            rust_integer_type(target_kind) && rust_integer_type(value_kind) &&
            strcmp(target_kind, value_kind) != 0;
        bool wrapping_integer =
            (strcmp(target_kind, "byte") == 0 ||
             strcmp(target_kind, "uint32") == 0 ||
             strcmp(target_kind, "uint") == 0) &&
            strcmp(target_kind, value_kind) == 0;
        bool unchecked_signed_integer =
            (strcmp(target_kind, "int") == 0 ||
             strcmp(target_kind, "long") == 0 ||
             strcmp(target_kind, "int32") == 0) &&
            strcmp(target_kind, value_kind) == 0 &&
            json_string_property_equals(
                expr, "mutation_arithmetic_mode", "unchecked");
        if (!rust_integer_type(target_kind) || !rust_integer_type(value_kind) ||
            (!mixed_integer && !wrapping_integer && !unchecked_signed_integer &&
             !json_string_property_equals(expr, "mutation_arithmetic_mode", "checked")) ||
            (!json_string_property_equals(expr, "mutation_storage", "local") &&
             !checked_ref_parameter &&
             !json_boolean_property(
                 expr, "rust_by_value_wrapping_parameter_mutation") &&
             !iterator_binding_mutation))
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
        const char *operand_kind = NULL, *operand_type_kind = NULL;
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
        bool iterator_binding_mutation =
            rust_mark_iterator_binding_mutation(expr, child);
        operand_type_kind = json_string_property(operand_type, "kind");
        if (iterator_binding_mutation &&
            !rust_integer_type(operand_type_kind) &&
            !rust_float_type(operand_type_kind))
        {
            fprintf(stderr,
                    "Error: Rust target supports iterator-protocol increment/decrement only for int, long, int32, byte, uint32, uint, float, or double bindings\n");
            return false;
        }
        if (json_string_property_equals(expr, "mutation_storage", "parameter"))
        {
            if (!json_string_property_equals(child, "parameter_mem_qual", "as_ref") &&
                !json_boolean_property(
                    expr, "rust_by_value_floating_parameter_mutation") &&
                !json_boolean_property(
                    expr, "rust_by_value_wrapping_parameter_mutation") &&
                !iterator_binding_mutation)
            {
                fprintf(stderr,
                        "Error: Rust target does not support increment/decrement of by-value parameters\n");
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
            if (!json_string_property_equals(expr, "mutation_storage", "local") &&
                !rust_floating_ref_parameter(expr, child) &&
                !json_boolean_property(
                    expr, "rust_by_value_floating_parameter_mutation") &&
                !iterator_binding_mutation)
            {
                fprintf(stderr,
                        "Error: Rust target supports floating-point increment/decrement only for stable mutable locals and direct fields\n");
                return false;
            }
            return rust_validate_expr(child);
        }
        if (!json_string_property_equals(expr, "mutation_arithmetic_mode", "checked"))
        {
            if (iterator_binding_mutation &&
                strcmp(operand_type_kind, "byte") != 0 &&
                strcmp(operand_type_kind, "uint32") != 0 &&
                strcmp(operand_type_kind, "uint") != 0)
            {
                fprintf(stderr,
                        "Error: Rust target supports integer iterator-protocol increment/decrement only with checked arithmetic\n");
                return false;
            }
            return rust_validate_expr(child);
        }
        if (!json_string_property_equals(expr, "mutation_storage", "local") &&
            !rust_checked_scalar_ref_parameter(expr, child) &&
            !json_boolean_property(
                expr, "rust_by_value_wrapping_parameter_mutation") &&
            !iterator_binding_mutation)
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
        return rust_validate_call(expr);
    if (strcmp(kind, "builtin_print") == 0 || strcmp(kind, "builtin_println") == 0)
    {
        json_object *args = NULL;
        json_object_object_get_ex(expr, "args", &args);
        return rust_validate_expr_array(args);
    }
    if (strcmp(kind, "method_call") == 0 || strcmp(kind, "borrow_inferred_call") == 0)
        return rust_validate_resolved_call(expr);
    if (strcmp(kind, "lambda") == 0) return rust_validate_lambda(expr);
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

typedef enum
{
    RUST_MATCH_PATTERN_OK,
    RUST_MATCH_PATTERN_NOT_LITERAL,
    RUST_MATCH_PATTERN_NEGATIVE_UNSIGNED,
    RUST_MATCH_PATTERN_NOT_LOSSLESS
} RustMatchPatternStatus;

static bool rust_match_integral_type(const char *kind)
{
    return rust_integer_type(kind);
}

/* Mirror the shared match checker's currently accepted integer pairings.  The
 * target still validates the modeled value below: shared compatibility alone
 * is not permission to narrow, wrap, or reinterpret a literal. */
static bool rust_match_integer_types_shared_compatible(const char *subject_kind,
                                                       const char *pattern_kind)
{
    if (!rust_match_integral_type(subject_kind) ||
        !rust_match_integral_type(pattern_kind)) return false;
    if (strcmp(subject_kind, pattern_kind) == 0) return true;

    bool subject_is_byte = strcmp(subject_kind, "byte") == 0;
    bool pattern_is_byte = strcmp(pattern_kind, "byte") == 0;
    if (subject_is_byte || pattern_is_byte)
        return (subject_is_byte && strcmp(pattern_kind, "int") == 0) ||
               (pattern_is_byte && strcmp(subject_kind, "int") == 0);

    /* The shared checker accepts every pairing among these five kinds. */
    return true;
}

static bool rust_match_literal_model_value(json_object *literal,
                                           const char *literal_kind,
                                           int64_t *value_out)
{
    json_object *value = NULL;
    const char *wanted_value_kind = strcmp(literal_kind, "byte") == 0
        ? "byte" : "int";
    if (!json_string_property_equals(literal, "kind", "literal") ||
        !json_string_property_equals(literal, "value_kind", wanted_value_kind) ||
        !json_object_object_get_ex(literal, "value", &value) ||
        !json_object_is_type(value, json_type_int)) return false;
    *value_out = json_object_get_int64(value);
    return true;
}

static bool rust_match_positive_literal_intrinsically_reliable(const char *kind,
                                                               int64_t value)
{
    if (value < 0) return false;
    if (strcmp(kind, "int") == 0 || strcmp(kind, "long") == 0 ||
        strcmp(kind, "uint") == 0) return true;
    if (strcmp(kind, "int32") == 0) return value <= INT32_MAX;
    if (strcmp(kind, "uint32") == 0) return (uint64_t)value <= UINT32_MAX;
    if (strcmp(kind, "byte") == 0) return value <= UINT8_MAX;
    return false;
}

static bool rust_match_positive_value_fits_subject(const char *subject_kind,
                                                   uint64_t value)
{
    if (strcmp(subject_kind, "int") == 0 ||
        strcmp(subject_kind, "long") == 0 ||
        strcmp(subject_kind, "uint") == 0)
        return value <= (uint64_t)INT64_MAX;
    if (strcmp(subject_kind, "int32") == 0)
        return value <= (uint64_t)INT32_MAX;
    if (strcmp(subject_kind, "uint32") == 0)
        return value <= (uint64_t)UINT32_MAX;
    if (strcmp(subject_kind, "byte") == 0)
        return value <= (uint64_t)UINT8_MAX;
    return false;
}

static bool rust_match_negative_magnitude_fits_subject(const char *subject_kind,
                                                       uint64_t magnitude)
{
    if (strcmp(subject_kind, "int") == 0 || strcmp(subject_kind, "long") == 0)
    {
        /* Source INT64_MIN is deliberately outside the reliable lexer/model
         * boundary, so the largest admitted magnitude remains INT64_MAX. */
        return magnitude <= (uint64_t)INT64_MAX;
    }
    if (strcmp(subject_kind, "int32") == 0)
        return magnitude <= (uint64_t)INT32_MAX + 1U;
    return false;
}

static RustMatchPatternStatus rust_integral_match_literal_pattern(
    json_object *pattern, const char *subject_kind)
{
    json_object *pattern_type = NULL;
    const char *pattern_kind = NULL;
    if (!json_object_is_type(pattern, json_type_object) ||
        !json_object_object_get_ex(pattern, "type", &pattern_type) ||
        !(pattern_kind = json_string_property(pattern_type, "kind")) ||
        !rust_match_integer_types_shared_compatible(subject_kind, pattern_kind))
        return RUST_MATCH_PATTERN_NOT_LITERAL;

    if (json_string_property_equals(pattern, "kind", "literal"))
    {
        int64_t value = 0;
        if (!rust_match_literal_model_value(pattern, pattern_kind, &value))
            return RUST_MATCH_PATTERN_NOT_LITERAL;
        /* A uint source literal above INT64_MAX is stored through the shared
         * signed model boundary as a negative number.  Never reinterpret it. */
        if (!rust_match_positive_literal_intrinsically_reliable(pattern_kind, value) ||
            !rust_match_positive_value_fits_subject(subject_kind, (uint64_t)value))
            return RUST_MATCH_PATTERN_NOT_LOSSLESS;
        return RUST_MATCH_PATTERN_OK;
    }

    json_object *operand = NULL, *operand_type = NULL;
    const char *operand_kind = NULL;
    int64_t magnitude = 0;
    if (!json_string_property_equals(pattern, "kind", "unary") ||
        !json_string_property_equals(pattern, "op", "negate") ||
        !json_object_object_get_ex(pattern, "operand", &operand) ||
        !json_object_is_type(operand, json_type_object) ||
        !json_object_object_get_ex(operand, "type", &operand_type) ||
        !(operand_kind = json_string_property(operand_type, "kind")) ||
        strcmp(pattern_kind, operand_kind) != 0 ||
        !rust_match_literal_model_value(operand, operand_kind, &magnitude))
        return RUST_MATCH_PATTERN_NOT_LITERAL;

    if (!rust_signed_integer_type(pattern_kind))
        return RUST_MATCH_PATTERN_NEGATIVE_UNSIGNED;
    if (rust_unsigned_integer_type(subject_kind))
        return RUST_MATCH_PATTERN_NEGATIVE_UNSIGNED;
    if (!rust_match_positive_literal_intrinsically_reliable(pattern_kind, magnitude) ||
        !rust_match_negative_magnitude_fits_subject(subject_kind,
                                                    (uint64_t)magnitude))
        return RUST_MATCH_PATTERN_NOT_LOSSLESS;
    return RUST_MATCH_PATTERN_OK;
}

static bool rust_report_integral_match_pattern_error(RustMatchPatternStatus status,
                                                     bool is_value_match)
{
    if (status == RUST_MATCH_PATTERN_NEGATIVE_UNSIGNED)
        return rust_report_match_error(
            is_value_match
                ? "does not support negative patterns for unsigned value-match subjects or unsigned literal suffixes"
                : "does not support negative patterns for unsigned statement-match subjects or unsigned literal suffixes");
    if (status == RUST_MATCH_PATTERN_NOT_LOSSLESS)
        return rust_report_match_error(
            is_value_match
                ? "requires integer literal patterns in value match to be losslessly representable in the subject type"
                : "requires integer literal patterns in statement match to be losslessly representable in the subject type");
    return rust_report_match_error(
        is_value_match
            ? "supports value match only with integer literal patterns"
            : "supports statement match only with integer literal patterns");
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

/* The shared optimizer folds recursively literal-only string concatenations
 * from -O1 onward.  Recognize that same bounded constant form here and attach
 * its content to the pattern, so Rust admission and rendering do not depend
 * on whether folding ran.  All other string expressions remain excluded. */
static char *rust_string_match_constant_pattern_value(json_object *pattern)
{
    json_object *type = NULL, *value = NULL;
    if (!json_object_is_type(pattern, json_type_object) ||
        !json_object_object_get_ex(pattern, "type", &type) ||
        !json_string_property_equals(type, "kind", "string"))
        return NULL;

    if (json_string_property_equals(pattern, "kind", "literal"))
    {
        if (!json_string_property_equals(pattern, "value_kind", "string") ||
            !json_object_object_get_ex(pattern, "value", &value) ||
            !json_object_is_type(value, json_type_string))
            return NULL;
        const char *text = json_object_get_string(value);
        return text ? strdup(text) : NULL;
    }

    if (json_string_property_equals(pattern, "kind", "str_concat_multi"))
    {
        json_object *parts = NULL;
        if (!json_object_object_get_ex(pattern, "parts", &parts) ||
            !json_object_is_type(parts, json_type_array) ||
            json_object_array_length(parts) == 0)
            return NULL;

        char *combined = strdup("");
        if (!combined) return NULL;
        size_t combined_len = 0;
        size_t part_count = json_object_array_length(parts);
        for (size_t i = 0; i < part_count; i++)
        {
            char *part = rust_string_match_constant_pattern_value(
                json_object_array_get_idx(parts, i));
            if (!part)
            {
                free(combined);
                return NULL;
            }
            size_t part_len = strlen(part);
            if (part_len == (size_t)-1 ||
                combined_len > (size_t)-1 - part_len - 1)
            {
                free(part);
                free(combined);
                return NULL;
            }
            char *grown = realloc(combined, combined_len + part_len + 1);
            if (!grown)
            {
                free(part);
                free(combined);
                return NULL;
            }
            combined = grown;
            memcpy(combined + combined_len, part, part_len + 1);
            combined_len += part_len;
            free(part);
        }
        return combined;
    }

    json_object *left = NULL, *right = NULL;
    if (!json_string_property_equals(pattern, "kind", "binary") ||
        !json_string_property_equals(pattern, "op", "add") ||
        !json_object_object_get_ex(pattern, "left", &left) ||
        !json_object_object_get_ex(pattern, "right", &right))
        return NULL;

    char *left_value = rust_string_match_constant_pattern_value(left);
    char *right_value = rust_string_match_constant_pattern_value(right);
    if (!left_value || !right_value)
    {
        free(left_value);
        free(right_value);
        return NULL;
    }

    size_t left_len = strlen(left_value);
    size_t right_len = strlen(right_value);
    if (right_len == (size_t)-1 ||
        left_len > (size_t)-1 - right_len - 1)
    {
        free(left_value);
        free(right_value);
        return NULL;
    }
    char *combined = malloc(left_len + right_len + 1);
    if (combined)
    {
        memcpy(combined, left_value, left_len);
        memcpy(combined + left_len, right_value, right_len + 1);
    }
    free(left_value);
    free(right_value);
    return combined;
}

static bool rust_prepare_string_match_pattern(json_object *pattern)
{
    char *value = rust_string_match_constant_pattern_value(pattern);
    if (value)
    {
        json_object_object_add(pattern, "rust_string_pattern_value",
                               json_object_new_string(value));
        free(value);
        return true;
    }

    /* Dynamic string patterns are deliberately limited to stable borrowed
     * places.  Requiring a variable root excludes temporaries, calls, static
     * access, indexing, and every computed form while permitting arbitrarily
     * deep field access through locals, parameters, and self. */
    json_object *type = NULL;
    if (!json_object_is_type(pattern, json_type_object) ||
        !json_object_object_get_ex(pattern, "type", &type) ||
        !json_string_property_equals(type, "kind", "string"))
        return false;

    json_object *root = pattern;
    while (json_string_property_equals(root, "kind", "member"))
    {
        if (!json_object_object_get_ex(root, "object", &root) ||
            !json_object_is_type(root, json_type_object))
            return false;
    }
    if (!json_string_property_equals(root, "kind", "variable")) return false;

    json_object_object_add(pattern, "rust_string_pattern_borrowed",
                           json_object_new_boolean(true));
    return true;
}

/* Keep result-form string calls inside the established read-only call
 * envelope. Mutating array methods may themselves return a string element;
 * mutating struct methods carry the target-local receiver analysis completed
 * before method bodies are validated. */
static bool rust_string_match_result_call_is_mutating(json_object *expr)
{
    if (rust_is_mutating_array_call(expr)) return true;
    if (!json_string_property_equals(expr, "kind", "call")) return false;

    json_object *callee = NULL, *object = NULL, *object_type = NULL;
    if (!json_object_object_get_ex(expr, "callee", &callee) ||
        !json_string_property_equals(callee, "kind", "member") ||
        !json_object_object_get_ex(callee, "object", &object) ||
        !json_object_object_get_ex(object, "type", &object_type)) return false;

    if (json_string_property_equals(object_type, "kind", "pointer"))
    {
        json_object *base_type = NULL;
        if (!json_object_object_get_ex(object_type, "base_type", &base_type))
            return false;
        object_type = base_type;
    }
    if (!json_string_property_equals(object_type, "kind", "struct")) return false;

    json_object *structure = rust_find_struct(
        rust_validation_model, json_string_property(object_type, "name"));
    json_object *methods = NULL;
    const char *called_name = json_string_property(callee, "member_name");
    if (!structure || !called_name ||
        !json_object_object_get_ex(structure, "methods", &methods)) return false;
    size_t method_count = json_object_array_length(methods);
    for (size_t i = 0; i < method_count; i++)
    {
        json_object *method = json_object_array_get_idx(methods, i);
        if (json_string_property_equals(method, "name", called_name))
            return json_boolean_property(method, "rust_mutating");
    }
    return false;
}

/* Generic array access currently renders its receiver more than once in both
 * production backends. Admit only recursively stable places/indices here;
 * stabilizing effectful result receivers belongs to the shared expression
 * family and is intentionally outside this target-local slice. */
static bool rust_string_match_result_access_is_stable(json_object *expr)
{
    const char *kind = json_string_property(expr, "kind");
    if (!kind) return false;
    if (strcmp(kind, "literal") == 0 || strcmp(kind, "variable") == 0)
        return true;
    if (strcmp(kind, "member") == 0)
    {
        json_object *object = NULL;
        return json_object_object_get_ex(expr, "object", &object) &&
               rust_string_match_result_access_is_stable(object);
    }
    if (strcmp(kind, "array_access") == 0)
    {
        json_object *array = NULL, *index = NULL;
        return json_object_object_get_ex(expr, "array", &array) &&
               json_object_object_get_ex(expr, "index", &index) &&
               rust_string_match_result_access_is_stable(array) &&
               rust_string_match_result_access_is_stable(index);
    }
    return false;
}

typedef enum
{
    RUST_FLOAT_MATCH_PATTERN_OK,
    RUST_FLOAT_MATCH_PATTERN_NOT_LITERAL,
    RUST_FLOAT_MATCH_PATTERN_WRONG_TYPE
} RustFloatMatchPatternStatus;

static RustFloatMatchPatternStatus rust_float_match_literal_pattern(
    json_object *pattern, const char *subject_kind)
{
    json_object *pattern_type = NULL;
    const char *pattern_kind = NULL;
    if (!json_object_is_type(pattern, json_type_object) ||
        !json_object_object_get_ex(pattern, "type", &pattern_type) ||
        !(pattern_kind = json_string_property(pattern_type, "kind")) ||
        !rust_float_type(pattern_kind))
        return RUST_FLOAT_MATCH_PATTERN_NOT_LITERAL;
    if (strcmp(pattern_kind, subject_kind) != 0)
        return RUST_FLOAT_MATCH_PATTERN_WRONG_TYPE;

    json_object *literal = pattern;
    if (json_string_property_equals(pattern, "kind", "unary"))
    {
        json_object *operand_type = NULL;
        if (!json_string_property_equals(pattern, "op", "negate") ||
            !json_object_object_get_ex(pattern, "operand", &literal) ||
            !json_object_is_type(literal, json_type_object) ||
            !json_object_object_get_ex(literal, "type", &operand_type) ||
            !json_string_property_equals(operand_type, "kind", subject_kind))
            return RUST_FLOAT_MATCH_PATTERN_NOT_LITERAL;
    }

    json_object *value = NULL;
    if (!json_string_property_equals(literal, "kind", "literal") ||
        !json_string_property_equals(literal, "value_kind", "double") ||
        !json_object_object_get_ex(literal, "value", &value) ||
        !json_object_is_type(value, json_type_double))
        return RUST_FLOAT_MATCH_PATTERN_NOT_LITERAL;
    return RUST_FLOAT_MATCH_PATTERN_OK;
}

static bool rust_report_float_match_pattern_error(
    RustFloatMatchPatternStatus status, bool is_value_match)
{
    if (status == RUST_FLOAT_MATCH_PATTERN_WRONG_TYPE)
        return rust_report_match_error(
            is_value_match
                ? "requires floating literal patterns in value match to have the exact subject type"
                : "requires floating literal patterns in statement match to have the exact subject type");
    return rust_report_match_error(
        is_value_match
            ? "supports floating value match only with floating literal patterns"
            : "supports floating statement match only with floating literal patterns");
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
    const char *subject_kind = json_string_property(subject_type, "kind");
    bool subject_is_integral = rust_match_integral_type(subject_kind);
    bool subject_is_bool = json_string_property_equals(subject_type, "kind", "bool");
    bool subject_is_float = rust_float_type(subject_kind);
    bool subject_is_string = json_string_property_equals(subject_type, "kind", "string");
    if (!subject_is_integral && !subject_is_bool && !subject_is_float &&
        !subject_is_string)
    {
        return rust_report_match_error(
            "supports statement match only with bool, integral, float, double, or string subjects");
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
                if (subject_is_integral)
                {
                    RustMatchPatternStatus status =
                        rust_integral_match_literal_pattern(pattern, subject_kind);
                    if (status != RUST_MATCH_PATTERN_OK)
                        return rust_report_integral_match_pattern_error(status, false);
                }
                if (subject_is_bool && !rust_bool_match_literal_pattern(pattern))
                    return rust_report_match_error(
                        "supports statement match only with boolean literal patterns");
                if (subject_is_float)
                {
                    RustFloatMatchPatternStatus status =
                        rust_float_match_literal_pattern(pattern, subject_kind);
                    if (status != RUST_FLOAT_MATCH_PATTERN_OK)
                        return rust_report_float_match_pattern_error(status, false);
                }
                if (subject_is_string &&
                    !rust_prepare_string_match_pattern(pattern))
                    return rust_report_match_error(
                        "supports string statement match only with string literal, literal-only concatenation, or stable borrowed variable/member patterns");
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
    if (subject_is_float)
        json_object_object_add(expr, "rust_floating_match",
                               json_object_new_boolean(true));
    if (subject_is_string)
        json_object_object_add(expr, "rust_string_match",
                               json_object_new_boolean(true));
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

    const char *subject_kind = json_string_property(subject_type, "kind");
    bool subject_is_integral = rust_match_integral_type(subject_kind);
    bool subject_is_bool = json_string_property_equals(subject_type, "kind", "bool");
    bool subject_is_float = rust_float_type(subject_kind);
    bool subject_is_string = json_string_property_equals(subject_type, "kind", "string");
    if (!subject_is_integral && !subject_is_bool && !subject_is_float &&
        !subject_is_string)
        return rust_report_match_error(
            "supports value match only with bool, integral, float, double, or string subjects");

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
                json_object *pattern = json_object_array_get_idx(patterns, p);
                if (subject_is_integral)
                {
                    RustMatchPatternStatus status =
                        rust_integral_match_literal_pattern(pattern, subject_kind);
                    if (status != RUST_MATCH_PATTERN_OK)
                        return rust_report_integral_match_pattern_error(status, true);
                }
                if (subject_is_bool && !rust_bool_match_literal_pattern(pattern))
                    return rust_report_match_error(
                        "supports value match only with boolean literal patterns");
                if (subject_is_float)
                {
                    RustFloatMatchPatternStatus status =
                        rust_float_match_literal_pattern(pattern, subject_kind);
                    if (status != RUST_FLOAT_MATCH_PATTERN_OK)
                        return rust_report_float_match_pattern_error(status, true);
                }
                if (subject_is_string &&
                    !rust_prepare_string_match_pattern(pattern))
                    return rust_report_match_error(
                        "supports string value match only with string literal, literal-only concatenation, or stable borrowed variable/member patterns");
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
    const char *result_kind = json_string_property(result_type, "kind");
    if (!result_kind ||
        (!rust_match_integral_type(result_kind) &&
         !rust_float_type(result_kind) && strcmp(result_kind, "bool") != 0 &&
         strcmp(result_kind, "string") != 0))
        return rust_report_match_error(
            "supports value match results only for exact str or heap-free scalar bool, int, long, int32, uint32, uint, byte, float, or double");

    for (size_t i = 0; i < arm_count; i++)
    {
        json_object *arm = json_object_array_get_idx(arms, i);
        json_object *body = NULL, *body_statements = NULL;
        if (!json_object_object_get_ex(arm, "body", &body) ||
            !json_object_object_get_ex(body, "statements", &body_statements))
            return rust_report_match_error("encountered malformed value match model");

        size_t statement_count = json_object_array_length(body_statements);
        if (statement_count == 0)
            return rust_report_match_error(
                "requires each value match arm body to be nonempty");

        for (size_t s = 0; s + 1 < statement_count; s++)
        {
            json_object *prefix = json_object_array_get_idx(body_statements, s);
            if (!json_string_property_equals(prefix, "kind", "expr"))
                return rust_report_match_error(
                    "requires every value match arm prefix to be an expression statement");
        }

        json_object *statement =
            json_object_array_get_idx(body_statements, statement_count - 1);
        json_object *arm_expr = NULL, *arm_type = NULL;
        if (!json_string_property_equals(statement, "kind", "expr") ||
            !json_object_object_get_ex(statement, "expr", &arm_expr) ||
            !json_object_is_type(arm_expr, json_type_object) ||
            !json_object_object_get_ex(arm_expr, "type", &arm_type) ||
            !json_string_property_equals(arm_type, "kind", result_kind))
        {
            if (strcmp(result_kind, "string") == 0)
                return rust_report_match_error(
                    "requires each value match arm body to end with an exact str result expression");
            if (strcmp(result_kind, "int") == 0)
                return rust_report_match_error(
                    "requires each value match arm body to end with an exact int result expression");
            if (strcmp(result_kind, "bool") == 0)
                return rust_report_match_error(
                    "requires each value match arm body to end with an exact bool result expression");
            return rust_report_match_error(
                "requires each value match arm body to end with an expression of the exact resolved result type");
        }
        if (strcmp(result_kind, "string") == 0)
        {
            const char *arm_kind = json_string_property(arm_expr, "kind");
            bool supported = arm_kind &&
                (strcmp(arm_kind, "literal") == 0 ||
                 strcmp(arm_kind, "variable") == 0 ||
                 strcmp(arm_kind, "member") == 0 ||
                 strcmp(arm_kind, "array_access") == 0 ||
                 strcmp(arm_kind, "binary") == 0 ||
                 strcmp(arm_kind, "str_concat_multi") == 0 ||
                 strcmp(arm_kind, "interpolated_string") == 0 ||
                 strcmp(arm_kind, "call") == 0 ||
                 strcmp(arm_kind, "static_call") == 0 ||
                 strcmp(arm_kind, "match") == 0);
            if (!supported)
                return rust_report_match_error(
                    "supports str value-match results only for literals, borrowed variables, members, indexed elements, concatenation, interpolation, supported non-mutating calls, or nested str-result matches");

            if (strcmp(arm_kind, "array_access") == 0 &&
                !rust_string_match_result_access_is_stable(arm_expr))
                return rust_report_match_error(
                    "requires indexed str value-match results to use a stable local/member receiver and stable indices");

            /* Array mutation methods can return an element and therefore have
             * exact str type for str[]. Their effect/value ordering remains a
             * separate semantic family, not an implicit part of this slice. */
            if (rust_string_match_result_call_is_mutating(arm_expr))
                return rust_report_match_error(
                    "does not support mutating calls as str value-match results");
        }
    }

    /* Do not recurse into any executable child until the complete match
     * topology, result family, and every arm's prefix/tail shape are known to
     * be admissible. This keeps structural diagnostics ahead of failures in
     * earlier prefixes while retaining source evaluation order afterwards. */
    if (!rust_validate_expr(subject)) return false;
    for (size_t i = 0; i < arm_count; i++)
    {
        json_object *arm = json_object_array_get_idx(arms, i);
        json_object *body = NULL, *body_statements = NULL;
        if (!json_object_object_get_ex(arm, "body", &body) ||
            !json_object_object_get_ex(body, "statements", &body_statements))
            return rust_report_match_error("encountered malformed value match model");

        size_t statement_count = json_object_array_length(body_statements);
        for (size_t s = 0; s + 1 < statement_count; s++)
            if (!rust_validate_stmt(
                    json_object_array_get_idx(body_statements, s))) return false;

        json_object *statement =
            json_object_array_get_idx(body_statements, statement_count - 1);
        json_object *arm_expr = NULL;
        if (!json_object_object_get_ex(statement, "expr", &arm_expr))
            return rust_report_match_error("encountered malformed value match model");
        if (!rust_validate_expr(arm_expr)) return false;
    }

    json_object_object_add(expr, "rust_value_match", json_object_new_boolean(true));
    if (subject_is_float)
        json_object_object_add(expr, "rust_floating_match",
                               json_object_new_boolean(true));
    if (subject_is_string)
        json_object_object_add(expr, "rust_string_match",
                               json_object_new_boolean(true));
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

    if (!rust_validate_expr(iterable)) return false;

    const char *iterator_name = json_string_property(stmt, "iterator_name");
    if (!iterator_name)
    {
        fprintf(stderr,
                "Error: Rust target encountered malformed iterator-protocol foreach model\n");
        return false;
    }
    RustIteratorBindingScope scope = {iterator_name, rust_iterator_binding_scope};
    rust_iterator_binding_scope = &scope;
    bool body_valid = rust_validate_block(body);
    rust_iterator_binding_scope = scope.parent;
    return body_valid;
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
static bool rust_validate_model_impl(json_object *model,
                                     const RustNativePlan *native_plan)
{
    const char *unsupported = NULL;
    if (!array_is_empty(model, "globals")) unsupported = "global variables";
    else if (!rust_validate_closures(model)) return false;
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
                if (!rust_native_plan_has_work(native_plan) && value &&
                    (strcmp(value, "source") == 0 || strcmp(value, "include") == 0))
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
            bool native = json_object_object_get_ex(function, "is_native", &is_native) &&
                          json_object_get_boolean(is_native);
            if ((native && !rust_native_validate_declaration(native_plan, function)) ||
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
            if (native) continue;
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

static bool rust_validate_model(json_object *model, ArithmeticMode arithmetic_mode,
                                const RustNativePlan *native_plan)
{
    rust_validation_model = model;
    rust_validation_reported_error = false;
    rust_validation_arithmetic_mode = arithmetic_mode;
    rust_iterator_binding_scope = NULL;
    bool valid = rust_validate_model_impl(model, native_plan);
    rust_validation_model = NULL;
    rust_validation_reported_error = false;
    rust_iterator_binding_scope = NULL;
    return valid;
}
