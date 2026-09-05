#include "target/rust/rust_native.h"
#include "target/rust/rust_native_internal.h"
#include "cgen/gen_model_split.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#ifdef _WIN32
#include <stdlib.h>
#else
#include <unistd.h>
#endif

typedef struct {
    char *rust_callable_name;
    char *c_link_symbol;
} RustNativeDeclaration;

struct RustNativePlan {
    ModularModel *split;
    RustNativeDeclaration *declarations;
    size_t declaration_count;
};

/* Shared privately by the Rust-native rendering/build translation units. */
ModularModel *rust_native_plan_split(RustNativePlan *plan)
{
    return plan ? plan->split : NULL;
}

static const char *native_string(json_object *object, const char *key)
{
    json_object *value = NULL;
    return object && json_object_object_get_ex(object, key, &value)
        ? json_object_get_string(value) : NULL;
}

static bool native_bool(json_object *object, const char *key)
{
    json_object *value = NULL;
    return object && json_object_object_get_ex(object, key, &value) &&
           json_object_get_boolean(value);
}

static bool native_scalar_kind(const char *kind, bool allow_void)
{
    if (!kind) return false;
    return (allow_void && strcmp(kind, "void") == 0) ||
        strcmp(kind, "int") == 0 || strcmp(kind, "long") == 0 ||
        strcmp(kind, "int32") == 0 || strcmp(kind, "uint") == 0 ||
        strcmp(kind, "uint32") == 0 || strcmp(kind, "byte") == 0 ||
        strcmp(kind, "float") == 0 || strcmp(kind, "double") == 0;
}

static bool native_scalar_type(json_object *type, bool allow_void)
{
    return native_scalar_kind(native_string(type, "kind"), allow_void);
}

static bool native_body_has_unsupported_construct_impl(json_object *node,
                                                        bool direct_callee)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (native_body_has_unsupported_construct_impl(
                    json_object_array_get_idx(node, i), false)) return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;

    const char *kind = native_string(node, "kind");
    if ((kind && (strcmp(kind, "lambda") == 0 ||
                  strcmp(kind, "closure_call") == 0 ||
                  strcmp(kind, "array_literal") == 0 ||
                  strcmp(kind, "sized_array") == 0 ||
                  strcmp(kind, "array_access") == 0 ||
                  strcmp(kind, "array_slice") == 0 ||
                  strcmp(kind, "struct_literal") == 0 ||
                  strcmp(kind, "thread_spawn") == 0 ||
                  strcmp(kind, "thread_sync") == 0 ||
                  strcmp(kind, "thread_detach") == 0 ||
                  strncmp(kind, "thread_", 7) == 0)) ||
        native_bool(node, "is_closure_call"))
        return true;

    json_object *type = NULL;
    if (json_object_object_get_ex(node, "type", &type))
    {
        const char *type_kind = native_string(type, "kind");
        if (type_kind && (strcmp(type_kind, "string") == 0 ||
                          strcmp(type_kind, "array") == 0 ||
                          strcmp(type_kind, "struct") == 0 ||
                          strcmp(type_kind, "pointer") == 0 ||
                          (strcmp(type_kind, "function") == 0 && !direct_callee)))
            return true;
    }

    json_object_object_foreach(node, key, value)
    {
        bool child_is_direct_callee = kind && strcmp(kind, "call") == 0 &&
                                      strcmp(key, "callee") == 0;
        if (native_body_has_unsupported_construct_impl(
                value, child_is_direct_callee)) return true;
    }
    return false;
}

static bool native_body_has_unsupported_construct(json_object *node)
{
    return native_body_has_unsupported_construct_impl(node, false);
}

static bool validate_native_function(json_object *function)
{
    const char *name = native_string(function, "name");
    json_object *return_type = NULL, *params = NULL, *body = NULL;
    if (native_bool(function, "is_variadic"))
    {
        fprintf(stderr,
                "Error: Rust target native function '%s' cannot be variadic yet\n",
                name ? name : "<anonymous>");
        return false;
    }
    if (!json_object_object_get_ex(function, "return_type", &return_type) ||
        !native_scalar_type(return_type, true))
    {
        const char *kind = native_string(return_type, "kind");
        fprintf(stderr,
                "Error: Rust target native function '%s' has unsupported result type '%s'; the native scalar bridge supports void, int, long, int32, uint, uint32, byte, float, and double\n",
                name ? name : "<anonymous>", kind ? kind : "unknown");
        return false;
    }
    if (json_object_object_get_ex(function, "params", &params))
    {
        size_t count = json_object_array_length(params);
        for (size_t i = 0; i < count; i++)
        {
            json_object *param = json_object_array_get_idx(params, i);
            json_object *type = NULL;
            const char *mem = native_string(param, "mem_qual");
            const char *sync = native_string(param, "sync_mod");
            if (!json_object_object_get_ex(param, "type", &type) ||
                !native_scalar_type(type, false) ||
                (mem && strcmp(mem, "default") != 0) ||
                (sync && strcmp(sync, "none") != 0))
            {
                fprintf(stderr,
                        "Error: Rust target native function '%s' parameter '%s' must be an unsynchronized, default-qualified native scalar\n",
                        name ? name : "<anonymous>",
                        native_string(param, "name") ? native_string(param, "name") : "<anonymous>");
                return false;
            }
        }
    }
    if (json_object_object_get_ex(function, "body", &body) &&
        native_body_has_unsupported_construct(body))
    {
        fprintf(stderr,
                "Error: Rust target native function '%s' body uses a closure, thread, pointer, string, array, or struct construct outside the native scalar bridge\n",
                name ? name : "<anonymous>");
        return false;
    }
    return true;
}

static json_object *deep_copy(json_object *source)
{
    json_object *copy = NULL;
    return source && json_object_deep_copy(source, &copy, NULL) == 0 ? copy : NULL;
}

static void replace_with_empty_array(json_object *object, const char *key)
{
    json_object_object_del(object, key);
    json_object_object_add(object, key, json_object_new_array());
}

static bool function_matches_callable(json_object *function,
                                      const char *callable_name)
{
    if (!function || !callable_name) return false;
    const char *name = native_string(function, "name");
    const char *source_name = native_string(function, "source_callable_name");
    return (name && strcmp(name, callable_name) == 0) ||
           (source_name && strcmp(source_name, callable_name) == 0);
}

static void mark_function_dependencies(json_object *node,
                                       json_object *functions,
                                       bool *selected,
                                       size_t function_count,
                                       bool *changed)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            mark_function_dependencies(json_object_array_get_idx(node, i),
                                       functions, selected, function_count,
                                       changed);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    const char *kind = native_string(node, "kind");
    if (kind && strcmp(kind, "call") == 0)
    {
        json_object *callee = NULL;
        if (json_object_object_get_ex(node, "callee", &callee) &&
            callee && native_string(callee, "kind") &&
            strcmp(native_string(callee, "kind"), "variable") == 0)
        {
            const char *callable_name = native_string(callee, "name");
            for (size_t i = 0; callable_name && i < function_count; i++)
            {
                json_object *function = json_object_array_get_idx(functions, i);
                if (!selected[i] && function_matches_callable(function, callable_name))
                {
                    selected[i] = true;
                    *changed = true;
                    break;
                }
            }
        }
    }

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        mark_function_dependencies(value, functions, selected, function_count,
                                   changed);
    }
}

static void mark_named_model_dependencies(json_object *node,
                                          json_object *structs,
                                          bool *selected_structs,
                                          size_t struct_count,
                                          json_object *globals,
                                          bool *selected_globals,
                                          size_t global_count,
                                          bool *changed)
{
    /* This walks only nodes already selected for the Rust-private sidecar.
     * Repeating it with the callable walk below computes a deterministic
     * source-order closure without exposing dependency metadata to C codegen. */
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            mark_named_model_dependencies(json_object_array_get_idx(node, i),
                                          structs, selected_structs,
                                          struct_count, globals,
                                          selected_globals, global_count,
                                          changed);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    const char *kind = native_string(node, "kind");
    const char *name = native_string(node, "name");
    if (kind && name && strcmp(kind, "struct") == 0)
    {
        for (size_t i = 0; i < struct_count; i++)
        {
            const char *struct_name = native_string(
                json_object_array_get_idx(structs, i), "name");
            if (!selected_structs[i] && struct_name &&
                strcmp(struct_name, name) == 0)
            {
                selected_structs[i] = true;
                *changed = true;
                break;
            }
        }
    }
    else if (kind && name && strcmp(kind, "variable") == 0)
    {
        for (size_t i = 0; i < global_count; i++)
        {
            const char *global_name = native_string(
                json_object_array_get_idx(globals, i), "name");
            if (!selected_globals[i] && global_name &&
                strcmp(global_name, name) == 0)
            {
                selected_globals[i] = true;
                *changed = true;
                break;
            }
        }
    }

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        mark_named_model_dependencies(value, structs, selected_structs,
                                      struct_count, globals,
                                      selected_globals, global_count, changed);
    }
}

static json_object *selected_array(json_object *source, const bool *selected,
                                   size_t count)
{
    json_object *result = json_object_new_array();
    if (!result) return NULL;
    for (size_t i = 0; i < count; i++)
        if (selected[i])
            json_object_array_add(result, json_object_get(
                json_object_array_get_idx(source, i)));
    return result;
}

static bool node_references_selected_global(json_object *node,
                                            json_object *globals,
                                            const bool *selected_globals,
                                            size_t global_count)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (node_references_selected_global(
                    json_object_array_get_idx(node, i), globals,
                    selected_globals, global_count)) return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;
    const char *kind = native_string(node, "kind");
    const char *name = native_string(node, "name");
    if (kind && name && strcmp(kind, "variable") == 0)
    {
        for (size_t i = 0; i < global_count; i++)
        {
            const char *global_name = native_string(
                json_object_array_get_idx(globals, i), "name");
            if (selected_globals[i] && global_name &&
                strcmp(global_name, name) == 0) return true;
        }
    }
    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (node_references_selected_global(value, globals, selected_globals,
                                            global_count)) return true;
    }
    return false;
}

static bool node_calls_selected_function(json_object *node,
                                         json_object *functions,
                                         const bool *selected_functions,
                                         size_t function_count)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (node_calls_selected_function(
                    json_object_array_get_idx(node, i), functions,
                    selected_functions, function_count)) return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;
    if (native_string(node, "kind") &&
        strcmp(native_string(node, "kind"), "call") == 0)
    {
        json_object *callee = NULL;
        if (json_object_object_get_ex(node, "callee", &callee) &&
            callee && native_string(callee, "kind") &&
            strcmp(native_string(callee, "kind"), "variable") == 0)
        {
            const char *callable_name = native_string(callee, "name");
            for (size_t i = 0; callable_name && i < function_count; i++)
                if (selected_functions[i] && function_matches_callable(
                        json_object_array_get_idx(functions, i), callable_name))
                    return true;
        }
    }
    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (node_calls_selected_function(value, functions,
                                         selected_functions, function_count))
            return true;
    }
    return false;
}

static json_object *native_primitive_type(const char *kind)
{
    json_object *type = json_object_new_object();
    if (type) json_object_object_add(type, "kind", json_object_new_string(kind));
    return type;
}

static json_object *native_assignment_statement(const char *target,
                                                json_object *value)
{
    json_object *assignment = json_object_new_object();
    json_object *statement = json_object_new_object();
    if (!assignment || !statement)
    {
        if (assignment) json_object_put(assignment);
        if (statement) json_object_put(statement);
        return NULL;
    }
    json_object_object_add(assignment, "kind", json_object_new_string("assign"));
    json_object_object_add(assignment, "target", json_object_new_string(target));
    json_object_object_add(assignment, "value", value);
    json_object_object_add(statement, "kind", json_object_new_string("expr"));
    json_object_object_add(statement, "expr", assignment);
    return statement;
}

static bool model_name_in_use(json_object *functions, json_object *globals,
                              const char *candidate)
{
    json_object *collections[] = {functions, globals};
    for (size_t collection = 0; collection < 2; collection++)
    {
        json_object *items = collections[collection];
        size_t count = items ? json_object_array_length(items) : 0;
        for (size_t i = 0; i < count; i++)
        {
            json_object *item = json_object_array_get_idx(items, i);
            const char *name = native_string(item, "name");
            const char *source_name = native_string(item, "source_callable_name");
            const char *alias = native_string(item, "c_alias");
            const char *link_symbol = native_string(item, "c_link_symbol");
            if ((name && strcmp(name, candidate) == 0) ||
                (source_name && strcmp(source_name, candidate) == 0) ||
                (alias && (strcmp(alias, candidate) == 0 ||
                           (strncmp(alias, "__sn__", 6) == 0 &&
                            strcmp(alias + 6, candidate) == 0))) ||
                (link_symbol && (strcmp(link_symbol, candidate) == 0 ||
                                 (strncmp(link_symbol, "__sn__", 6) == 0 &&
                                  strcmp(link_symbol + 6, candidate) == 0))))
                return true;
        }
    }
    return false;
}

static char *unique_private_name(json_object *functions, json_object *globals,
                                 const char *stem)
{
    for (size_t suffix = 0; suffix < SIZE_MAX; suffix++)
    {
        int needed = snprintf(NULL, 0, "%s_%zu", stem, suffix);
        if (needed < 0) return NULL;
        char *candidate = malloc((size_t)needed + 1);
        if (!candidate) return NULL;
        snprintf(candidate, (size_t)needed + 1, "%s_%zu", stem, suffix);
        if (!model_name_in_use(functions, globals, candidate)) return candidate;
        free(candidate);
    }
    return NULL;
}

static bool add_native_initializer(json_object *functions,
                                   json_object *globals,
                                   const char *initializer_name)
{
    /* The C backend normally assigns deferred globals from generated main().
     * A sidecar has no C main, so synthesize the equivalent routine. The Rust
     * main invokes it exactly once before any source main statements. */
    json_object *initializer = json_object_new_object();
    json_object *body = json_object_new_array();
    json_object *params = json_object_new_array();
    if (!initializer || !body || !params || !initializer_name) goto fail;

    size_t global_count = json_object_array_length(globals);
    for (size_t i = 0; i < global_count; i++)
    {
        json_object *global = json_object_array_get_idx(globals, i);
        if (!native_bool(global, "is_deferred")) continue;
        json_object *value = NULL;
        const char *name = native_string(global, "name");
        if (!name || !json_object_object_get_ex(global, "initializer", &value))
            continue;
        json_object *assignment = native_assignment_statement(name, deep_copy(value));
        if (!assignment) goto fail;
        json_object_array_add(body, assignment);
    }

    json_object_object_add(initializer, "name",
                           json_object_new_string(initializer_name));
    json_object_object_add(initializer, "return_type", native_primitive_type("void"));
    json_object_object_add(initializer, "params", params);
    params = NULL;
    json_object_object_add(initializer, "body", body);
    body = NULL;
    json_object_object_add(initializer, "has_body", json_object_new_boolean(true));
    json_object_object_add(initializer, "is_native", json_object_new_boolean(false));
    json_object_array_add(functions, initializer);
    return true;

fail:
    if (initializer) json_object_put(initializer);
    if (body) json_object_put(body);
    if (params) json_object_put(params);
    return false;
}

static bool project_native_model(json_object *model,
                                 json_object **selected_function_names,
                                 json_object **selected_global_names,
                                 char **initializer_name_out)
{
    json_object *functions = NULL, *structs = NULL, *globals = NULL;
    json_object_object_get_ex(model, "functions", &functions);
    json_object_object_get_ex(model, "structs", &structs);
    json_object_object_get_ex(model, "globals", &globals);
    size_t function_count = functions ? json_object_array_length(functions) : 0;
    size_t struct_count = structs ? json_object_array_length(structs) : 0;
    size_t global_count = globals ? json_object_array_length(globals) : 0;
    bool *selected_functions = function_count
        ? calloc(function_count, sizeof(*selected_functions)) : NULL;
    bool *selected_structs = struct_count
        ? calloc(struct_count, sizeof(*selected_structs)) : NULL;
    bool *selected_globals = global_count
        ? calloc(global_count, sizeof(*selected_globals)) : NULL;
    if ((function_count && !selected_functions) ||
        (struct_count && !selected_structs) ||
        (global_count && !selected_globals))
    {
        free(selected_functions);
        free(selected_structs);
        free(selected_globals);
        return false;
    }
    for (size_t i = 0; i < function_count; i++)
        if (native_bool(json_object_array_get_idx(functions, i), "is_native"))
            selected_functions[i] = true;

    bool changed;
    do
    {
        changed = false;
        for (size_t i = 0; i < function_count; i++)
        {
            if (!selected_functions[i]) continue;
            json_object *function = json_object_array_get_idx(functions, i);
            mark_function_dependencies(function, functions,
                                       selected_functions, function_count,
                                       &changed);
            mark_named_model_dependencies(function, structs, selected_structs,
                                          struct_count, globals,
                                          selected_globals, global_count,
                                          &changed);
        }
        for (size_t i = 0; i < struct_count; i++)
        {
            if (!selected_structs[i]) continue;
            json_object *structure = json_object_array_get_idx(structs, i);
            mark_function_dependencies(structure, functions,
                                       selected_functions, function_count,
                                       &changed);
            mark_named_model_dependencies(structure, structs,
                                          selected_structs, struct_count,
                                          globals, selected_globals,
                                          global_count, &changed);
        }
        for (size_t i = 0; i < global_count; i++)
        {
            if (!selected_globals[i]) continue;
            json_object *global = json_object_array_get_idx(globals, i);
            mark_function_dependencies(global, functions,
                                       selected_functions, function_count,
                                       &changed);
            mark_named_model_dependencies(global, structs, selected_structs,
                                          struct_count, globals,
                                          selected_globals, global_count,
                                          &changed);
        }
    } while (changed);

    json_object *native_functions = selected_array(
        functions, selected_functions, function_count);
    json_object *native_structs = selected_array(
        structs, selected_structs, struct_count);
    json_object *native_globals = selected_array(
        globals, selected_globals, global_count);
    json_object *function_names = json_object_new_array();
    json_object *global_names = json_object_new_array();
    bool has_deferred_global = false;
    bool *private_helpers = function_count
        ? calloc(function_count, sizeof(*private_helpers)) : NULL;
    if (!native_functions || !native_structs || !native_globals ||
        !function_names || !global_names ||
        (function_count && !private_helpers))
    {
        if (native_functions) json_object_put(native_functions);
        if (native_structs) json_object_put(native_structs);
        if (native_globals) json_object_put(native_globals);
        if (function_names) json_object_put(function_names);
        if (global_names) json_object_put(global_names);
        free(selected_functions);
        free(selected_structs);
        free(selected_globals);
        free(private_helpers);
        return false;
    }
    for (size_t i = 0; i < function_count; i++)
        if (selected_functions[i] &&
            !native_bool(json_object_array_get_idx(functions, i), "is_native") &&
            node_references_selected_global(
                json_object_array_get_idx(functions, i), globals,
                selected_globals, global_count))
            private_helpers[i] = true;
    /* Deferred initializers execute on the C side. Keep every ordinary helper
     * they call, and its transitive callees, out of Rust validation. */
    for (size_t i = 0; i < global_count; i++)
        if (selected_globals[i])
            mark_function_dependencies(json_object_array_get_idx(globals, i),
                                       functions, private_helpers,
                                       function_count, &changed);
    do
    {
        changed = false;
        for (size_t i = 0; i < function_count; i++)
            if (private_helpers[i])
                mark_function_dependencies(json_object_array_get_idx(functions, i),
                                           functions, private_helpers,
                                           function_count, &changed);
    } while (changed);
    do
    {
        changed = false;
        for (size_t i = 0; i < function_count; i++)
        {
            if (!selected_functions[i] || private_helpers[i] ||
                native_bool(json_object_array_get_idx(functions, i), "is_native"))
                continue;
            if (node_calls_selected_function(
                    json_object_array_get_idx(functions, i), functions,
                    private_helpers, function_count))
            {
                private_helpers[i] = true;
                changed = true;
            }
        }
    } while (changed);
    for (size_t i = 0; i < function_count; i++)
    {
        if (!private_helpers[i]) continue;
        const char *name = native_string(
            json_object_array_get_idx(functions, i), "name");
        if (name) json_object_array_add(function_names, json_object_new_string(name));
    }
    free(private_helpers);
    for (size_t i = 0; i < global_count; i++)
    {
        if (!selected_globals[i]) continue;
        json_object *global = json_object_array_get_idx(globals, i);
        const char *name = native_string(global, "name");
        if (name) json_object_array_add(global_names, json_object_new_string(name));
        if (native_bool(global, "is_deferred")) has_deferred_global = true;
    }
    free(selected_functions);
    free(selected_structs);
    free(selected_globals);
    char *initializer_name = NULL;
    if (has_deferred_global)
        initializer_name = unique_private_name(functions, globals,
                                               "__rust_native_initialize");
    if (has_deferred_global && (!initializer_name ||
        !add_native_initializer(native_functions, native_globals,
                                initializer_name)))
    {
        free(initializer_name);
        json_object_put(native_functions);
        json_object_put(native_structs);
        json_object_put(native_globals);
        json_object_put(function_names);
        json_object_put(global_names);
        return false;
    }
    json_object_object_del(model, "functions");
    json_object_object_add(model, "functions", native_functions);
    json_object_object_del(model, "structs");
    json_object_object_add(model, "structs", native_structs);
    json_object_object_del(model, "globals");
    json_object_object_add(model, "globals", native_globals);
    *selected_function_names = function_names;
    *selected_global_names = global_names;
    *initializer_name_out = initializer_name;
    replace_with_empty_array(model, "lambdas");
    replace_with_empty_array(model, "threads");
    replace_with_empty_array(model, "fn_wrappers");
    replace_with_empty_array(model, "type_decls");
    replace_with_empty_array(model, "top_level_statements");

    json_object *module = NULL;
    if (json_object_object_get_ex(model, "module", &module))
    {
        json_object_object_add(module, "has_main", json_object_new_boolean(false));
        json_object_object_add(module, "has_main_args", json_object_new_boolean(false));
        json_object_object_add(module, "main_returns", json_object_new_boolean(false));
    }
    return true;
}

static void resolve_private_include_origins(json_object *model)
{
    json_object *pragmas = NULL;
    if (!json_object_object_get_ex(model, "pragmas", &pragmas)) return;
    size_t count = json_object_array_length(pragmas);
    for (size_t i = 0; i < count; i++)
    {
        json_object *pragma = json_object_array_get_idx(pragmas, i);
        if (!native_string(pragma, "pragma_type") ||
            strcmp(native_string(pragma, "pragma_type"), "include") != 0)
            continue;
        const char *value = native_string(pragma, "value");
        const char *origin = native_string(pragma, "source_dir");
        size_t length = value ? strlen(value) : 0;
        if (!origin || length < 2 || value[0] != '"' || value[length - 1] != '"')
            continue;

        char relative[PATH_MAX];
        int written = snprintf(relative, sizeof(relative), "%s/%.*s", origin,
                               (int)(length - 2), value + 1);
        if (written < 0 || (size_t)written >= sizeof(relative)) continue;
        char resolved[PATH_MAX];
#ifdef _WIN32
        if (!_fullpath(resolved, relative, sizeof(resolved))) continue;
        for (char *p = resolved; *p; p++) if (*p == '\\') *p = '/';
#else
        if (!realpath(relative, resolved)) continue;
#endif
        char quoted[PATH_MAX + 3];
        written = snprintf(quoted, sizeof(quoted), "\"%s\"", resolved);
        if (written < 0 || (size_t)written >= sizeof(quoted)) continue;
        json_object_object_add(pragma, "value", json_object_new_string(quoted));
    }
}

static void restore_source_callable_names(json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            restore_source_callable_names(json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    const char *callable = native_string(node, "source_callable_name");
    if (callable && native_string(node, "kind"))
        json_object_object_add(node, "name", json_object_new_string(callable));
    json_object_object_foreach(node, key, value)
    {
        (void)key;
        restore_source_callable_names(value);
    }
}

static void remove_private_globals(json_object *rust_model,
                                   json_object *selected_names)
{
    json_object *globals = NULL;
    if (!rust_model || !selected_names ||
        !json_object_object_get_ex(rust_model, "globals", &globals)) return;
    json_object *remaining = json_object_new_array();
    if (!remaining) return;
    size_t global_count = json_object_array_length(globals);
    size_t selected_count = json_object_array_length(selected_names);
    for (size_t i = 0; i < global_count; i++)
    {
        json_object *global = json_object_array_get_idx(globals, i);
        const char *name = native_string(global, "name");
        bool selected = false;
        for (size_t j = 0; name && j < selected_count; j++)
        {
            const char *selected_name = json_object_get_string(
                json_object_array_get_idx(selected_names, j));
            if (selected_name && strcmp(name, selected_name) == 0)
            {
                selected = true;
                break;
            }
        }
        if (!selected) json_object_array_add(remaining, json_object_get(global));
    }
    json_object_object_del(rust_model, "globals");
    json_object_object_add(rust_model, "globals", remaining);
}

static void remove_private_helper_functions(json_object *rust_model,
                                            json_object *selected_names)
{
    /* Struct-only helpers may still be useful to ordinary Rust code and can
     * safely remain duplicated. Helpers that read privatized globals cannot
     * remain in Rust, whose central validator intentionally rejects globals. */
    json_object *functions = NULL;
    if (!rust_model || !selected_names ||
        !json_object_object_get_ex(rust_model, "functions", &functions)) return;
    json_object *remaining = json_object_new_array();
    if (!remaining) return;
    size_t function_count = json_object_array_length(functions);
    size_t selected_count = json_object_array_length(selected_names);
    for (size_t i = 0; i < function_count; i++)
    {
        json_object *function = json_object_array_get_idx(functions, i);
        const char *name = native_string(function, "name");
        bool selected = false;
        for (size_t j = 0; !native_bool(function, "is_native") && name &&
                           j < selected_count; j++)
        {
            const char *selected_name = json_object_get_string(
                json_object_array_get_idx(selected_names, j));
            if (selected_name && strcmp(name, selected_name) == 0)
            {
                selected = true;
                break;
            }
        }
        if (!selected) json_object_array_add(remaining, json_object_get(function));
    }
    json_object_object_del(rust_model, "functions");
    json_object_object_add(rust_model, "functions", remaining);
}

bool rust_native_partition_model(json_object *rust_model,
                                 const CompilerOptions *options,
                                 RustNativePlan **out_plan)
{
    if (!out_plan) return false;
    *out_plan = NULL;
    if (!rust_model || !options) return false;

    json_object *functions = NULL;
    size_t native_count = 0;
    if (json_object_object_get_ex(rust_model, "functions", &functions))
    {
        size_t count = json_object_array_length(functions);
        for (size_t i = 0; i < count; i++)
        {
            json_object *function = json_object_array_get_idx(functions, i);
            if (!native_bool(function, "is_native")) continue;
            if (!validate_native_function(function)) return false;
            native_count++;
        }
    }

    json_object *private_model = deep_copy(rust_model);
    RustNativePlan *plan = calloc(1, sizeof(*plan));
    if (!private_model || !plan)
    {
        if (private_model) json_object_put(private_model);
        free(plan);
        return false;
    }
    resolve_private_include_origins(private_model);
    json_object *selected_function_names = NULL;
    json_object *selected_global_names = NULL;
    char *initializer_name = NULL;
    if (!project_native_model(private_model, &selected_function_names,
                              &selected_global_names,
                              &initializer_name))
    {
        json_object_put(private_model);
        free(plan);
        return false;
    }
    plan->split = gen_model_split(private_model, options->source_file);
    json_object_put(private_model);
    if (!plan->split)
    {
        json_object_put(selected_function_names);
        json_object_put(selected_global_names);
        free(initializer_name);
        rust_native_plan_free(plan);
        return false;
    }
    if (native_count)
    {
        plan->declarations = calloc(native_count, sizeof(*plan->declarations));
        if (!plan->declarations)
        {
            json_object_put(selected_function_names);
            json_object_put(selected_global_names);
            free(initializer_name);
            rust_native_plan_free(plan);
            return false;
        }
    }
    plan->declaration_count = native_count;

    char *initializer_symbol = NULL;
    char *rust_initializer_name = NULL;
    char *rust_fflush_name = NULL;
    if (initializer_name)
    {
        initializer_symbol = malloc(strlen(initializer_name) + 7);
        json_object *globals = NULL;
        json_object_object_get_ex(rust_model, "globals", &globals);
        rust_initializer_name = unique_private_name(
            functions, globals, "__sn_native_initializer");
        rust_fflush_name = unique_private_name(
            functions, globals, "__sn_native_fflush");
        if (!initializer_symbol || !rust_initializer_name || !rust_fflush_name)
        {
            free(initializer_symbol);
            free(rust_initializer_name);
            free(rust_fflush_name);
            json_object_put(selected_function_names);
            json_object_put(selected_global_names);
            free(initializer_name);
            rust_native_plan_free(plan);
            return false;
        }
        snprintf(initializer_symbol, strlen(initializer_name) + 7,
                 "__sn__%s", initializer_name);
    }

    /* Finish all fallible owned-plan allocation before touching rust_model, so
     * a partition failure cannot leave a partially annotated Rust projection. */
    size_t native_index = 0;
    if (functions)
    {
        size_t count = json_object_array_length(functions);
        for (size_t i = 0; i < count; i++)
        {
            json_object *function = json_object_array_get_idx(functions, i);
            if (!native_bool(function, "is_native")) continue;
            const char *name = native_string(function, "source_callable_name");
            if (!name) name = native_string(function, "name");
            const char *alias = native_string(function, "c_alias");
            const char *symbol = !native_bool(function, "has_body") && alias
                ? alias : name;
            plan->declarations[native_index].rust_callable_name = strdup(name ? name : "");
            plan->declarations[native_index].c_link_symbol = strdup(symbol ? symbol : "");
            if (!plan->declarations[native_index].rust_callable_name ||
                !plan->declarations[native_index].c_link_symbol)
            {
                json_object_put(selected_function_names);
                json_object_put(selected_global_names);
                free(initializer_symbol);
                free(rust_initializer_name);
                free(rust_fflush_name);
                free(initializer_name);
                rust_native_plan_free(plan);
                return false;
            }
            native_index++;
        }
    }

    native_index = 0;
    if (functions)
    {
        size_t count = json_object_array_length(functions);
        for (size_t i = 0; i < count; i++)
        {
            json_object *function = json_object_array_get_idx(functions, i);
            if (!native_bool(function, "is_native")) continue;
            const char *name = plan->declarations[native_index].rust_callable_name;
            const char *symbol = plan->declarations[native_index].c_link_symbol;
            char bridge[64];
            snprintf(bridge, sizeof(bridge), "__sn_native_%zu", native_index);
            json_object_object_add(function, "rust_native_bridge",
                                   json_object_new_boolean(true));
            json_object_object_add(function, "rust_native_bridge_id",
                                   json_object_new_int64((int64_t)native_index));
            json_object_object_add(function, "rust_native_extern_name",
                                   json_object_new_string(bridge));
            json_object_object_add(function, "rust_callable_name",
                                   json_object_new_string(name));
            json_object_object_add(function, "c_link_symbol",
                                   json_object_new_string(symbol));
            json_object_object_del(function, "body");
            json_object_object_add(function, "body", json_object_new_array());
            json_object_object_add(function, "has_body", json_object_new_boolean(false));
            native_index++;
        }
    }

    remove_private_helper_functions(rust_model, selected_function_names);
    remove_private_globals(rust_model, selected_global_names);
    if (initializer_name)
    {
        json_object *remaining_functions = NULL;
        json_object_object_get_ex(rust_model, "functions", &remaining_functions);
        json_object_object_add(rust_model, "rust_native_initializer",
                               json_object_new_boolean(true));
        json_object_object_add(rust_model, "rust_native_initializer_symbol",
                               json_object_new_string(initializer_symbol));
        json_object_object_add(rust_model, "rust_native_initializer_extern_name",
                               json_object_new_string(rust_initializer_name));
        json_object_object_add(rust_model, "rust_native_fflush_extern_name",
                               json_object_new_string(rust_fflush_name));
        size_t count = remaining_functions
            ? json_object_array_length(remaining_functions) : 0;
        for (size_t i = 0; i < count; i++)
        {
            json_object *function = json_object_array_get_idx(remaining_functions, i);
            if (native_string(function, "name") &&
                strcmp(native_string(function, "name"), "main") == 0)
            {
                json_object_object_add(function, "rust_native_initializer",
                                       json_object_new_boolean(true));
                json_object_object_add(function,
                                       "rust_native_initializer_extern_name",
                                       json_object_new_string(rust_initializer_name));
                json_object_object_add(function, "rust_native_fflush_extern_name",
                                       json_object_new_string(rust_fflush_name));
                break;
            }
        }
    }
    json_object_put(selected_function_names);
    json_object_put(selected_global_names);
    free(initializer_symbol);
    free(rust_initializer_name);
    free(rust_fflush_name);
    free(initializer_name);
    restore_source_callable_names(rust_model);

    *out_plan = plan;
    return true;
}

bool rust_native_validate_declaration(const RustNativePlan *plan,
                                      json_object *function)
{
    json_object *id_object = NULL;
    if (!plan || !native_bool(function, "rust_native_bridge") ||
        !json_object_object_get_ex(function, "rust_native_bridge_id", &id_object))
        return false;
    int64_t raw_id = json_object_get_int64(id_object);
    if (raw_id < 0 || (uint64_t)raw_id >= plan->declaration_count) return false;
    size_t id = (size_t)raw_id;
    const char *rust_name = native_string(function, "rust_callable_name");
    const char *symbol = native_string(function, "c_link_symbol");
    return rust_name && symbol &&
        strcmp(rust_name, plan->declarations[id].rust_callable_name) == 0 &&
        strcmp(symbol, plan->declarations[id].c_link_symbol) == 0 &&
        validate_native_function(function);
}

bool rust_native_plan_has_work(const RustNativePlan *plan)
{
    return plan && (plan->declaration_count > 0 ||
        (plan->split && (plan->split->source_file_count > 0 ||
                         plan->split->link_lib_count > 0)));
}

void rust_native_plan_free(void *opaque)
{
    RustNativePlan *plan = opaque;
    if (!plan) return;
    modular_model_free(plan->split);
    for (size_t i = 0; i < plan->declaration_count; i++)
    {
        free(plan->declarations[i].rust_callable_name);
        free(plan->declarations[i].c_link_symbol);
    }
    free(plan->declarations);
    free(plan);
}
