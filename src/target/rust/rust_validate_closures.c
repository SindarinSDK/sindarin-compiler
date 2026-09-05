/* Private lexical capture validation. Shared scalar cells and weak recursive
 * identity are explicit; Rust implicit captures never decide source ownership. */
static json_object *rust_closure_property(json_object *node, const char *key)
{
    json_object *value = NULL;
    if (node) json_object_object_get_ex(node, key, &value);
    return value;
}

static size_t rust_closure_length(json_object *array)
{
    return array && json_object_is_type(array, json_type_array)
        ? json_object_array_length(array) : 0;
}

static bool rust_closure_error(const char *reason)
{
    fprintf(stderr, "Error: Rust target does not support %s yet\n", reason);
    rust_validation_reported_error = true;
    return false;
}

static bool rust_closure_scalar_type(json_object *type)
{
    const char *kind = json_string_property(type, "kind");
    return kind && (strcmp(kind, "int") == 0 || strcmp(kind, "long") == 0 ||
        strcmp(kind, "int32") == 0 || strcmp(kind, "byte") == 0 ||
        strcmp(kind, "uint32") == 0 || strcmp(kind, "uint") == 0 ||
        strcmp(kind, "float") == 0 || strcmp(kind, "double") == 0 ||
        strcmp(kind, "bool") == 0 || strcmp(kind, "char") == 0);
}

static bool rust_closure_owned_type(json_object *type)
{
    if (!rust_type_supported(type)) return false;
    if (json_string_property_equals(type, "kind", "struct"))
        return rust_auto_copy_plain_value_struct_type(type, NULL);
    if (json_string_property_equals(type, "kind", "array"))
        return rust_closure_owned_type(rust_closure_property(type, "element_type"));
    return !json_string_property_equals(type, "kind", "void");
}

static bool rust_closure_type_supported(json_object *type)
{
    if (!json_string_property_equals(type, "kind", "function")) return false;
    if (json_boolean_property(type, "is_native") ||
        json_boolean_property(type, "is_variadic") ||
        json_boolean_property(type, "has_arena_param")) return false;
    json_object *quals = rust_closure_property(type, "param_mem_quals");
    for (size_t i = 0; i < rust_closure_length(quals); i++)
        if (strcmp(json_object_get_string(json_object_array_get_idx(quals, i)), "default") != 0)
            return false;
    json_object *params = rust_closure_property(type, "param_types");
    if (!params) return false;
    for (size_t i = 0; i < rust_closure_length(params); i++)
    {
        json_object *param = json_object_array_get_idx(params, i);
        if (!rust_closure_owned_type(param)) return false;
        if (json_string_property_equals(param, "kind", "struct") &&
            !rust_heap_free_named_struct_type(param)) return false;
    }
    json_object *ret = rust_closure_property(type, "return_type");
    return json_string_property_equals(ret, "kind", "void") || rust_closure_owned_type(ret);
}

/* Exact recursive shape matters even where the shared checker permits a
 * float/double substitution. Metadata such as pass_by_ptr is not a type. */
static bool rust_closure_same_type(json_object *a, json_object *b)
{
    const char *ak = json_string_property(a, "kind");
    const char *bk = json_string_property(b, "kind");
    if (!ak || !bk || strcmp(ak, bk) != 0) return false;
    if (strcmp(ak, "struct") == 0)
        return json_string_property_equals(a, "name", json_string_property(b, "name"));
    if (strcmp(ak, "array") == 0)
        return rust_closure_same_type(rust_closure_property(a, "element_type"),
                                      rust_closure_property(b, "element_type"));
    if (strcmp(ak, "function") != 0) return true;
    if (!rust_closure_type_supported(a) || !rust_closure_type_supported(b)) return false;
    json_object *ap = rust_closure_property(a, "param_types");
    json_object *bp = rust_closure_property(b, "param_types");
    if (rust_closure_length(ap) != rust_closure_length(bp)) return false;
    for (size_t i = 0; i < rust_closure_length(ap); i++)
        if (!rust_closure_same_type(json_object_array_get_idx(ap, i),
                                   json_object_array_get_idx(bp, i))) return false;
    return rust_closure_same_type(rust_closure_property(a, "return_type"),
                                 rust_closure_property(b, "return_type"));
}

/* Binding identity is independent of source spelling and of C's name-based
 * promotion flags. Scope copies keep shadowing and nested captures explicit. */
typedef struct RustClosureBinding {
    const char *name;
    json_object *declaration;
    int id;
    int lambda_depth;
    bool capture;
    struct RustClosureBinding *next;
} RustClosureBinding;

typedef struct {
    RustClosureBinding *bindings;
    int next_id;
    int lambda_depth;
    json_object *model;
    json_object *return_type;
} RustClosureScope;

static RustClosureBinding *rust_closure_lookup(RustClosureScope *scope, const char *name)
{
    for (RustClosureBinding *b = scope->bindings; b; b = b->next)
        if (name && strcmp(name, b->name) == 0) return b;
    return NULL;
}

static void rust_closure_pop(RustClosureScope *scope, RustClosureBinding *saved)
{
    while (scope->bindings != saved)
    {
        RustClosureBinding *b = scope->bindings;
        scope->bindings = b->next;
        free(b);
    }
}

static bool rust_closure_bind(RustClosureScope *scope, json_object *node,
                              const char *name, int id, bool capture)
{
    if (!name) return false;
    RustClosureBinding *b = malloc(sizeof(*b));
    if (!b) return false;
    *b = (RustClosureBinding){name, node, id < 0 ? scope->next_id++ : id,
                             scope->lambda_depth, capture, scope->bindings};
    scope->bindings = b;
    if (!capture && json_string_property_equals(node, "kind", "var_decl") &&
        json_boolean_property(node, "is_captured") &&
        rust_closure_scalar_type(rust_closure_property(node, "type")) &&
        json_string_property_equals(node, "mem_qual", "default"))
        json_object_object_add(node, "rust_shared_cell", json_object_new_boolean(true));
    json_object_object_add(node, "rust_binding_id", json_object_new_int(b->id));
    return true;
}

static bool rust_closure_named_function(RustClosureScope *scope, const char *name)
{
    json_object *functions = rust_closure_property(scope->model, "functions");
    for (size_t i = 0; i < rust_closure_length(functions); i++)
        if (json_string_property_equals(json_object_array_get_idx(functions, i), "name", name))
            return true;
    return false;
}

static bool rust_closure_walk(RustClosureScope *scope, json_object *node);

static bool rust_closure_walk_scope(RustClosureScope *scope, json_object *node)
{
    RustClosureBinding *saved = scope->bindings;
    bool ok = rust_closure_walk(scope, node);
    rust_closure_pop(scope, saved);
    return ok;
}

static bool rust_closure_walk_lambda(RustClosureScope *scope, json_object *node)
{
    if (!rust_closure_type_supported(rust_closure_property(node, "type")) ||
        json_boolean_property(node, "is_native"))
        return rust_closure_error("this closure signature (native, qualified, variadic or unsupported owned type)");
    json_object *caps = rust_closure_property(node, "captures");
    /* Resolve every source before installing capture aliases, so sibling
     * slots cannot accidentally resolve against an alias installed earlier. */
    for (size_t i = 0; i < rust_closure_length(caps); i++)
    {
        json_object *cap = json_object_array_get_idx(caps, i);
        if (json_boolean_property(cap, "is_self"))
        {
            json_object_object_add(cap, "rust_binding_id", json_object_new_int(scope->next_id++));
            json_object_object_add(cap, "rust_capture_mode", json_object_new_string("self"));
            json_object_object_add(node, "rust_recursive", json_object_new_boolean(true));
            json_object_object_add(scope->model, "rust_has_recursive_closures", json_object_new_boolean(true));
            continue;
        }
        if (json_boolean_property(cap, "is_ref") &&
            !rust_closure_scalar_type(rust_closure_property(cap, "type")))
            return rust_closure_error("shared mutable closure captures");
        RustClosureBinding *b = rust_closure_lookup(scope, json_string_property(cap, "name"));
        if (!b) return rust_closure_error("unresolved or recursive closure captures");
        if (b->lambda_depth != scope->lambda_depth)
            return rust_closure_error("missing transitive closure captures");
        if (json_string_property_equals(b->declaration, "mem_qual", "as_ref") ||
            (json_boolean_property(b->declaration, "is_captured") &&
             !json_boolean_property(b->declaration, "rust_shared_cell")))
            return rust_closure_error("borrowed or promoted closure captures");
        if (!rust_closure_owned_type(rust_closure_property(cap, "type")))
            return rust_closure_error("this closure capture type");
        json_object_object_add(cap, "rust_binding_id", json_object_new_int(b->id));
        bool shared = json_boolean_property(cap, "is_ref");
        if (shared && !json_boolean_property(b->declaration, "rust_shared_cell"))
            return rust_closure_error("shared captures without scalar cell storage");
        json_object_object_add(cap, "rust_capture_mode", json_object_new_string(shared ? "shared" : "value"));
        if (shared)
            json_object_object_add(cap, "rust_shared_cell", json_object_new_boolean(true));
        else if (rust_closure_scalar_type(rust_closure_property(cap, "type")))
            json_object_object_add(cap, "rust_scalar_snapshot", json_object_new_boolean(true));
        if (!shared && json_boolean_property(b->declaration, "rust_shared_cell"))
            json_object_object_add(cap, "rust_snapshot_cell_source", json_object_new_boolean(true));
        if (json_string_property_equals(b->declaration, "rust_capture_mode", "self"))
            json_object_object_add(cap, "rust_self_source", json_object_new_boolean(true));
    }
    RustClosureBinding *saved = scope->bindings;
    scope->lambda_depth++;
    json_object *saved_return = scope->return_type;
    scope->return_type = rust_closure_property(node, "return_type");
    bool ok = true;
    for (size_t i = 0; ok && i < rust_closure_length(caps); i++)
    {
        json_object *cap = json_object_array_get_idx(caps, i);
        ok = rust_closure_bind(scope, cap, json_string_property(cap, "name"),
            json_object_get_int(rust_closure_property(cap, "rust_binding_id")), true);
    }
    json_object *params = rust_closure_property(node, "params");
    for (size_t i = 0; ok && i < rust_closure_length(params); i++)
    {
        json_object *p = json_object_array_get_idx(params, i);
        if (!json_string_property_equals(p, "mem_qual", "default"))
            ok = rust_closure_error("qualified closure parameters");
        else ok = rust_closure_bind(scope, p, json_string_property(p, "name"), -1, false);
    }
    json_object *body_value = rust_closure_property(node, "body");
    if (ok && body_value && json_string_property_equals(scope->return_type, "kind", "function") &&
        !rust_closure_same_type(scope->return_type, rust_closure_property(body_value, "type")))
        ok = rust_closure_error("incompatible function-value signatures");
    if (ok) ok = rust_closure_walk(scope, rust_closure_property(node, "body")) &&
                 rust_closure_walk(scope, rust_closure_property(node, "body_stmts"));
    rust_closure_pop(scope, saved);
    scope->lambda_depth--;
    scope->return_type = saved_return;
    return ok;
}

static RustClosureBinding *rust_closure_place(RustClosureScope *scope, json_object *node)
{
    if (json_string_property_equals(node, "kind", "variable"))
        return rust_closure_lookup(scope, json_string_property(node, "name"));
    if (json_string_property_equals(node, "kind", "member"))
        return rust_closure_place(scope, rust_closure_property(node, "object"));
    if (json_string_property_equals(node, "kind", "array_access"))
        return rust_closure_place(scope, rust_closure_property(node, "array"));
    return NULL;
}

static bool rust_closure_walk(RustClosureScope *scope, json_object *node)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < rust_closure_length(node); i++)
            if (!rust_closure_walk(scope, json_object_array_get_idx(node, i))) return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;
    const char *kind = json_string_property(node, "kind");
    if (kind && strcmp(kind, "expr") == 0)
    {
        json_object *discarded = rust_closure_property(node, "expr");
        if (discarded) json_object_object_add(discarded, "rust_closure_discarded", json_object_new_boolean(true));
    }
    if (kind && (strcmp(kind, "member_assign") == 0 || strcmp(kind, "index_assign") == 0) &&
        json_string_property_equals(rust_closure_property(node, "type"), "kind", "function") &&
        !json_boolean_property(node, "rust_closure_discarded"))
        return rust_closure_error("consumed function field or index assignment results");
    if (kind && strcmp(kind, "binary") == 0 &&
        (json_string_property_equals(node, "op", "lt") || json_string_property_equals(node, "op", "lte") ||
         json_string_property_equals(node, "op", "gt") || json_string_property_equals(node, "op", "gte")) &&
        (json_string_property_equals(rust_closure_property(rust_closure_property(node, "left"), "type"), "kind", "function") ||
         json_string_property_equals(rust_closure_property(rust_closure_property(node, "right"), "type"), "kind", "function")))
        return rust_closure_error("ordered function-value comparisons");
    if (kind && strcmp(kind, "lambda") == 0) return rust_closure_walk_lambda(scope, node);
    json_object *edge_value = rust_closure_property(node, "value");
    json_object *expected = rust_closure_property(node, "type");
    if (kind && strcmp(kind, "var_decl") == 0) edge_value = rust_closure_property(node, "initializer");
    if (kind && strcmp(kind, "return") == 0) expected = scope->return_type;
    if (edge_value && json_string_property_equals(expected, "kind", "function") &&
        !rust_closure_same_type(expected, rust_closure_property(edge_value, "type")))
        return rust_closure_error("incompatible function-value signatures");
    if (kind && strcmp(kind, "struct_literal") == 0)
    {
        json_object *structure = rust_find_struct(scope->model, json_string_property(node, "struct_name"));
        json_object *decls = rust_closure_property(structure, "fields");
        json_object *fields = rust_closure_property(node, "fields");
        for (size_t i = 0; i < rust_closure_length(fields); i++)
        {
            json_object *field = json_object_array_get_idx(fields, i);
            for (size_t d = 0; d < rust_closure_length(decls); d++)
            {
                json_object *decl = json_object_array_get_idx(decls, d);
                json_object *wanted = rust_closure_property(decl, "type");
                if (json_string_property_equals(decl, "name", json_string_property(field, "name")) &&
                    json_string_property_equals(wanted, "kind", "function") &&
                    !rust_closure_same_type(wanted, rust_closure_property(rust_closure_property(field, "value"), "type")))
                    return rust_closure_error("incompatible function-value signatures");
            }
        }
    }
    if (kind && strcmp(kind, "array_literal") == 0)
    {
        json_object *element_type = rust_closure_property(expected, "element_type");
        json_object *elements = rust_closure_property(node, "elements");
        if (json_string_property_equals(element_type, "kind", "function"))
            for (size_t i = 0; i < rust_closure_length(elements); i++)
            {
                json_object *element = json_object_array_get_idx(elements, i);
                if (!json_string_property_equals(element, "kind", "spread") &&
                    !rust_closure_same_type(element_type, rust_closure_property(element, "type")))
                    return rust_closure_error("incompatible function-value signatures");
            }
    }
    if (kind && strcmp(kind, "sized_array") == 0 &&
        json_string_property_equals(rust_closure_property(node, "element_type"), "kind", "function"))
        return rust_closure_error("sized arrays of function values");
    if (kind && strcmp(kind, "var_decl") == 0)
    {
        if (!rust_closure_walk(scope, rust_closure_property(node, "initializer"))) return false;
        if (json_string_property_equals(rust_closure_property(node, "type"), "kind", "function") &&
            !rust_closure_property(node, "initializer"))
            return rust_closure_error("uninitialized function values");
        return rust_closure_bind(scope, node, json_string_property(node, "name"), -1, false);
    }
    if (kind && strcmp(kind, "variable") == 0)
    {
        const char *name = json_string_property(node, "name");
        RustClosureBinding *b = rust_closure_lookup(scope, name);
        if (b)
        {
            if (b->lambda_depth != scope->lambda_depth)
                return rust_closure_error("missing transitive closure captures");
            json_object_object_add(node, "rust_binding_id", json_object_new_int(b->id));
            if (b->capture && !json_boolean_property(node, "rust_capture_mutation_place"))
                json_object_object_add(node, "rust_needs_clone", json_object_new_boolean(true));
            if (json_boolean_property(b->declaration, "rust_shared_cell"))
                json_object_object_add(node, "rust_shared_cell", json_object_new_boolean(true));
            if (json_string_property_equals(b->declaration, "rust_capture_mode", "self"))
                json_object_object_add(node, "rust_self_read", json_object_new_boolean(true));
            if ((b->capture || json_boolean_property(b->declaration, "rust_shared_cell")) &&
                json_boolean_property(node, "is_ref_arg"))
                return rust_closure_error("mutable access to snapshot closure captures");
        }
        else if (!json_boolean_property(node, "rust_direct_callee") && name &&
                 json_string_property_equals(rust_closure_property(node, "type"), "kind", "function"))
        {
            if (!rust_closure_named_function(scope, name) || strcmp(name, "main") == 0 ||
                !rust_closure_type_supported(rust_closure_property(node, "type")))
                return rust_closure_error("this named function value");
            json_object_object_add(node, "rust_named_function_value", json_object_new_boolean(true));
        }
        return true;
    }
    RustClosureBinding *place = NULL;
    if (kind && strcmp(kind, "assign") == 0)
        place = rust_closure_lookup(scope, json_string_property(node, "target"));
    else if (kind && strcmp(kind, "compound_assign") == 0)
        place = rust_closure_place(scope, rust_closure_property(node, "target"));
    else if (kind && (strcmp(kind, "increment") == 0 || strcmp(kind, "decrement") == 0))
        place = rust_closure_place(scope, rust_closure_property(node, "operand"));
    else if (kind && strcmp(kind, "member_assign") == 0)
        place = rust_closure_place(scope, rust_closure_property(node, "object"));
    else if (kind && strcmp(kind, "index_assign") == 0)
        place = rust_closure_place(scope, rust_closure_property(node, "array"));
    if (place && scope->lambda_depth > 0 && !json_string_property(place->declaration, "kind") && !place->capture)
        return rust_closure_error("mutation of closure parameters");
    if (json_boolean_property(node, "is_ref_arg"))
    {
        RustClosureBinding *borrowed = rust_closure_place(scope, node);
        if (borrowed && borrowed->capture)
            return rust_closure_error("mutable access to snapshot closure captures");
    }
    if (place && json_boolean_property(place->declaration, "rust_scalar_snapshot"))
    {
        json_object_object_add(place->declaration, "rust_mutable_snapshot", json_object_new_boolean(true));
        if (kind && (strcmp(kind, "compound_assign") == 0 ||
                     strcmp(kind, "increment") == 0 || strcmp(kind, "decrement") == 0))
            json_object_object_add(node, "rust_snapshot_mutation", json_object_new_boolean(true));
        json_object *mutation_place = NULL;
        if (kind && strcmp(kind, "compound_assign") == 0)
            mutation_place = rust_closure_property(node, "target");
        else if (kind && (strcmp(kind, "increment") == 0 || strcmp(kind, "decrement") == 0))
            mutation_place = rust_closure_property(node, "operand");
        if (mutation_place)
            json_object_object_add(mutation_place, "rust_capture_mutation_place",
                                   json_object_new_boolean(true));
    }
    if (place && json_boolean_property(place->declaration, "rust_shared_cell"))
    {
        json_object_object_add(node, "rust_shared_cell", json_object_new_boolean(true));
        json_object_object_add(node, "rust_cell_name", json_object_new_string(place->name));
    }
    if (place && (place->capture || place->lambda_depth < scope->lambda_depth) &&
        !json_boolean_property(place->declaration, "rust_shared_cell") &&
        !json_boolean_property(place->declaration, "rust_scalar_snapshot"))
        return rust_closure_error("mutable access to snapshot closure captures");
    if (kind && strcmp(kind, "call") == 0)
    {
        json_object *callee = rust_closure_property(node, "callee");
        json_object *args = rust_closure_property(node, "args");
        json_object *params = rust_closure_property(rust_closure_property(callee, "type"), "param_types");
        for (size_t i = 0; i < rust_closure_length(args) && i < rust_closure_length(params); i++)
        {
            json_object *wanted = json_object_array_get_idx(params, i);
            json_object *arg = json_object_array_get_idx(args, i);
            if (json_string_property_equals(wanted, "kind", "function") &&
                !rust_closure_same_type(wanted, rust_closure_property(arg, "type")))
                return rust_closure_error("incompatible function-value signatures");
        }
        /* The shared flags can miss both computed callees and lexical
         * bindings shadowing a module function. Resolve variable identity
         * before classifying; leave ordinary member methods with their owner. */
        bool indirect = false;
        if (json_string_property_equals(rust_closure_property(callee, "type"), "kind", "function"))
        {
            if (json_string_property_equals(callee, "kind", "variable"))
                indirect = rust_closure_lookup(scope, json_string_property(callee, "name")) != NULL;
            else if (json_string_property_equals(callee, "kind", "member"))
                indirect = json_boolean_property(node, "is_fn_field_call") ||
                           json_boolean_property(node, "is_closure_call");
            else indirect = true;
        }
        json_object_object_add(node, "rust_closure_call", json_object_new_boolean(indirect));
        json_object_object_add(callee, "rust_direct_callee", json_object_new_boolean(!indirect));
        if (json_string_property_equals(callee, "kind", "member") &&
            !json_boolean_property(node, "is_fn_field_call"))
        {
            RustClosureBinding *b = rust_closure_place(scope, rust_closure_property(callee, "object"));
            if (b && b->capture)
                return rust_closure_error("method calls on snapshot closure captures");
            if (b && scope->lambda_depth > 0 && !json_string_property(b->declaration, "kind"))
                return rust_closure_error("method calls on closure parameters");
        }
    }
    if (kind && (strcmp(kind, "for_each") == 0 || strcmp(kind, "for_each_iter") == 0))
    {
        if (!rust_closure_walk(scope, rust_closure_property(node, "iterable"))) return false;
        RustClosureBinding *saved = scope->bindings;
        bool ok = rust_closure_bind(scope, node, json_string_property(node, "iterator_name"), -1, false) &&
                  rust_closure_walk_scope(scope, rust_closure_property(node, "body"));
        rust_closure_pop(scope, saved);
        return ok;
    }
    if (kind && strcmp(kind, "for") == 0)
    {
        RustClosureBinding *saved = scope->bindings;
        bool ok = rust_closure_walk(scope, rust_closure_property(node, "init")) &&
                  rust_closure_walk(scope, rust_closure_property(node, "condition")) &&
                  rust_closure_walk(scope, rust_closure_property(node, "increment")) &&
                  rust_closure_walk_scope(scope, rust_closure_property(node, "body"));
        rust_closure_pop(scope, saved);
        return ok;
    }
    json_object_object_foreach(node, key, value)
    {
        if (strcmp(key, "type") == 0 || strcmp(key, "return_type") == 0 ||
            strcmp(key, "target_type") == 0 || strncmp(key, "rust_", 5) == 0) continue;
        bool scoped = strcmp(key, "body") == 0 || strcmp(key, "then_body") == 0 ||
                      strcmp(key, "else_body") == 0 || strcmp(key, "statements") == 0;
        if (!(scoped ? rust_closure_walk_scope(scope, value) : rust_closure_walk(scope, value))) return false;
    }
    if (kind && strcmp(kind, "binary") == 0 &&
        (json_string_property_equals(node, "op", "eq") || json_string_property_equals(node, "op", "neq")))
    {
        json_object *left = rust_closure_property(node, "left");
        json_object *right = rust_closure_property(node, "right");
        if (json_boolean_property(left, "rust_named_function_value") &&
            json_boolean_property(right, "rust_named_function_value"))
        {
            /* C compares bare function symbols, not newly allocated wrapper
             * boxes. Do not intern conversions: distinct boxes stay distinct. */
            bool equal = json_string_property_equals(left, "name", json_string_property(right, "name"));
            if (json_string_property_equals(node, "op", "neq")) equal = !equal;
            json_object_object_add(node, "rust_named_function_comparison", json_object_new_boolean(equal));
        }
    }
    return true;
}

static bool rust_validate_closures(json_object *model)
{
    RustClosureScope scope = {.model = model};
    json_object *functions = rust_closure_property(model, "functions");
    for (size_t i = 0; i < rust_closure_length(functions); i++)
    {
        json_object *fn = json_object_array_get_idx(functions, i);
        scope.return_type = rust_closure_property(fn, "return_type");
        json_object *params = rust_closure_property(fn, "params");
        bool ok = true;
        for (size_t p = 0; ok && p < rust_closure_length(params); p++)
        {
            json_object *param = json_object_array_get_idx(params, p);
            ok = rust_closure_bind(&scope, param, json_string_property(param, "name"), -1, false);
        }
        if (ok) ok = rust_closure_walk(&scope, rust_closure_property(fn, "body"));
        rust_closure_pop(&scope, NULL);
        if (!ok) return false;
    }
    /* Method capture lifetimes need their own receiver contract. Keep that
     * boundary explicit until the function foundation is integrated. */
    json_object *structs = rust_closure_property(model, "structs");
    for (size_t s = 0; s < rust_closure_length(structs); s++)
    {
        json_object *st = json_object_array_get_idx(structs, s);
        json_object *methods = rust_closure_property(st, "methods");
        for (size_t m = 0; m < rust_closure_length(methods); m++)
        {
            json_object *method = json_object_array_get_idx(methods, m);
            scope.return_type = rust_closure_property(method, "return_type");
            json_object *params = rust_closure_property(method, "params");
            json_object *self = json_object_new_object();
            json_object_object_add(self, "mem_qual", json_object_new_string("as_ref"));
            bool ok = rust_closure_bind(&scope, self, "self", -1, false);
            for (size_t p = 0; ok && p < rust_closure_length(params); p++)
            {
                json_object *param = json_object_array_get_idx(params, p);
                ok = rust_closure_bind(&scope, param, json_string_property(param, "name"), -1, false);
            }
            if (ok) ok = rust_closure_walk(&scope, rust_closure_property(method, "body"));
            rust_closure_pop(&scope, NULL);
            json_object_put(self);
            if (!ok) return false;
        }
    }
    return true;
}

static bool rust_validate_lambda(json_object *expr)
{
    if (!rust_closure_type_supported(rust_closure_property(expr, "type")))
        return rust_closure_error("this closure signature (native, qualified, variadic or unsupported owned type)");
    json_object *caps = rust_closure_property(expr, "captures");
    for (size_t i = 0; i < rust_closure_length(caps); i++)
        if (!json_string_property(json_object_array_get_idx(caps, i), "rust_capture_mode"))
            return rust_closure_error("closures in this callable context");
    json_object *body = rust_closure_property(expr, "body");
    if (body)
    {
        if (!rust_validate_expr(body)) return false;
        const char *kind = json_string_property(body, "kind");
        if (kind && (strcmp(kind, "variable") == 0 || strcmp(kind, "member") == 0 ||
                     strcmp(kind, "array_access") == 0))
            json_object_object_add(expr, "rust_clone_body", json_object_new_boolean(true));
        return true;
    }
    return rust_validate_statements(rust_closure_property(expr, "body_stmts"));
}

/* Captured scalar places have C's unchecked storage annotation. Validate the
 * cell operation here without weakening ordinary parameter/place validation. */
static bool rust_validate_closure_cell_mutation(json_object *expr)
{
    bool compound = json_string_property_equals(expr, "kind", "compound_assign");
    json_object *place = rust_closure_property(expr, compound ? "target" : "operand");
    json_object *type = rust_closure_property(place, "type");
    const char *kind = json_string_property(type, "kind");
    if (!json_string_property_equals(place, "kind", "variable") ||
        !kind || (!rust_integer_type(kind) && !rust_float_type(kind)) ||
        json_boolean_property(expr, "mutation_sync"))
        return rust_closure_error("this shared scalar mutation");
    const char *op = compound ? json_string_property(expr, "op") :
        (json_string_property_equals(expr, "kind", "increment") ? "add" : "subtract");
    const char *method = NULL;
    if (op && strcmp(op, "add") == 0) method = "checked_add";
    else if (op && strcmp(op, "subtract") == 0) method = "checked_sub";
    else if (op && strcmp(op, "multiply") == 0) method = "checked_mul";
    else if (op && strcmp(op, "divide") == 0) method = "checked_div";
    else if (op && strcmp(op, "modulo") == 0 && !rust_float_type(kind)) method = "checked_rem";
    if (!method) return rust_closure_error("this shared scalar mutation operator");
    /* C's unchecked unsigned capture updates wrap at the scalar width. */
    if (strcmp(kind, "byte") == 0 || strcmp(kind, "uint32") == 0 || strcmp(kind, "uint") == 0)
    {
        if (strcmp(method, "checked_add") == 0) method = "wrapping_add";
        else if (strcmp(method, "checked_sub") == 0) method = "wrapping_sub";
        else if (strcmp(method, "checked_mul") == 0) method = "wrapping_mul";
        else if (strcmp(method, "checked_div") == 0) method = "wrapping_div";
        else method = "wrapping_rem";
        json_object_object_add(expr, "rust_cell_wrapping", json_object_new_boolean(true));
    }
    json_object_object_add(expr, "rust_cell_method", json_object_new_string(method));
    if (compound)
    {
        json_object *value = rust_closure_property(expr, "value");
        if (!rust_closure_same_type(type, rust_closure_property(value, "type")))
            return rust_closure_error("mixed-type shared scalar mutation");
        if (!rust_validate_expr(value)) return false;
    }
    return rust_validate_expr(place);
}

static bool rust_validate_closure_snapshot_mutation(json_object *expr)
{
    bool compound = json_string_property_equals(expr, "kind", "compound_assign");
    json_object *place = rust_closure_property(expr, compound ? "target" : "operand");
    json_object *type = rust_closure_property(place, "type");
    const char *kind = json_string_property(type, "kind");
    if (!json_string_property_equals(place, "kind", "variable") ||
        !kind || (!rust_integer_type(kind) && !rust_float_type(kind)) ||
        json_boolean_property(expr, "mutation_sync"))
        return rust_closure_error("this mutable scalar snapshot operation");

    if (compound)
    {
        json_object *value = rust_closure_property(expr, "value");
        if (!rust_closure_same_type(type, rust_closure_property(value, "type")))
            return rust_closure_error("mixed-type mutable scalar snapshot operation");
        if (rust_float_type(kind))
        {
            const char *op = json_string_property(expr, "op");
            if (!op || (strcmp(op, "add") != 0 && strcmp(op, "subtract") != 0 &&
                        strcmp(op, "multiply") != 0 && strcmp(op, "divide") != 0))
            {
                fprintf(stderr,
                        "Error: Rust target supports floating-point compound assignment only for +=, -=, *=, and /=\n");
                rust_validation_reported_error = true;
                return false;
            }
        }
        if (!rust_validate_expr(value)) return false;
    }
    if (rust_integer_type(kind))
    {
        bool checked = rust_validation_arithmetic_mode == ARITH_CHECKED &&
            json_string_property_equals(expr, "mutation_arithmetic_mode", "checked");
        const char *op = compound ? json_string_property(expr, "op") :
            (json_string_property_equals(expr, "kind", "increment") ? "add" : "subtract");
        const char *method = NULL;
        if (op && strcmp(op, "add") == 0) method = "checked_add";
        else if (op && strcmp(op, "subtract") == 0) method = "checked_sub";
        else if (op && strcmp(op, "multiply") == 0) method = "checked_mul";
        else if (op && strcmp(op, "divide") == 0) method = "checked_div";
        else if (op && strcmp(op, "modulo") == 0) method = "checked_rem";
        if (!method) return rust_closure_error("this mutable scalar snapshot operator");
        if (!checked && (strcmp(kind, "uint32") == 0 || strcmp(kind, "uint") == 0))
        {
            if (strcmp(method, "checked_add") == 0) method = "wrapping_add";
            else if (strcmp(method, "checked_sub") == 0) method = "wrapping_sub";
            else if (strcmp(method, "checked_mul") == 0) method = "wrapping_mul";
            else if (strcmp(method, "checked_div") == 0) method = "wrapping_div";
            else method = "wrapping_rem";
            json_object_object_add(expr, "rust_snapshot_wrapping",
                                   json_object_new_boolean(true));
        }
        json_object_object_add(expr, "rust_checked_method", json_object_new_string(method));
    }
    return rust_validate_expr(place);
}

static bool rust_validate_function_value(json_object *expr)
{
    (void)expr;
    return true;
}

static RustValidationResult rust_validate_closure_call(json_object *expr)
{
    if (!json_boolean_property(expr, "rust_closure_call")) return RUST_VALIDATION_UNHANDLED;
    json_object *callee = rust_closure_property(expr, "callee");
    json_object *type = rust_closure_property(callee, "type");
    if (!rust_closure_type_supported(type))
    {
        rust_closure_error("this closure call signature");
        return RUST_VALIDATION_UNSUPPORTED;
    }
    if (!rust_validate_expr(callee)) return RUST_VALIDATION_UNSUPPORTED;
    json_object *args = rust_closure_property(expr, "args");
    json_object *params = rust_closure_property(type, "param_types");
    if (rust_closure_length(args) != rust_closure_length(params))
        return RUST_VALIDATION_UNSUPPORTED;
    for (size_t i = 0; i < rust_closure_length(args); i++)
    {
        json_object *arg = json_object_array_get_idx(args, i);
        json_object *actual = rust_closure_property(arg, "type");
        json_object *wanted = json_object_array_get_idx(params, i);
        /* The shared checker equates float and double, but Rust Fn does not. */
        if (!rust_closure_same_type(actual, wanted) ||
            json_boolean_property(arg, "is_ref_arg"))
        {
            rust_closure_error("mixed-type or reference closure arguments");
            return RUST_VALIDATION_UNSUPPORTED;
        }
        if (!rust_validate_expr(arg)) return RUST_VALIDATION_UNSUPPORTED;
        const char *kind = json_string_property(arg, "kind");
        if (kind && (strcmp(kind, "variable") == 0 || strcmp(kind, "member") == 0 ||
                     strcmp(kind, "array_access") == 0))
            json_object_object_add(arg, "rust_closure_arg_clone", json_object_new_boolean(true));
    }
    json_object_object_add(expr, "rust_closure_call", json_object_new_boolean(true));
    return RUST_VALIDATION_SUPPORTED;
}
