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

static bool project_native_model(json_object *model)
{
    json_object *functions = NULL;
    json_object *native_functions = json_object_new_array();
    if (!native_functions) return false;
    if (json_object_object_get_ex(model, "functions", &functions))
    {
        size_t count = json_object_array_length(functions);
        for (size_t i = 0; i < count; i++)
        {
            json_object *function = json_object_array_get_idx(functions, i);
            if (native_bool(function, "is_native"))
                json_object_array_add(native_functions, json_object_get(function));
        }
    }
    json_object_object_del(model, "functions");
    json_object_object_add(model, "functions", native_functions);
    replace_with_empty_array(model, "globals");
    replace_with_empty_array(model, "structs");
    replace_with_empty_array(model, "lambdas");
    replace_with_empty_array(model, "threads");
    replace_with_empty_array(model, "fn_wrappers");
    replace_with_empty_array(model, "type_decls");
    replace_with_empty_array(model, "top_level");

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
    if (!project_native_model(private_model))
    {
        json_object_put(private_model);
        free(plan);
        return false;
    }
    plan->split = gen_model_split(private_model, options->source_file);
    json_object_put(private_model);
    if (!plan->split)
    {
        rust_native_plan_free(plan);
        return false;
    }
    if (native_count)
    {
        plan->declarations = calloc(native_count, sizeof(*plan->declarations));
        if (!plan->declarations)
        {
            rust_native_plan_free(plan);
            return false;
        }
    }
    plan->declaration_count = native_count;

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
