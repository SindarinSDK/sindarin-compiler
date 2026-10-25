#include "gen_model.h"
#include <stdlib.h>

typedef struct {
    const char *name;
    const char *kind;
    const char *symbol;
    char *original;
} PackageNativeBinding;

static json_object *binding_child(json_object *object, const char *key)
{
    json_object *value = NULL;
    if (object) json_object_object_get_ex(object, key, &value);
    return value;
}

static const char *binding_string(json_object *object, const char *key)
{
    json_object *value = binding_child(object, key);
    if (!value || !json_object_is_type(value, json_type_string)) return NULL;
    const char *text = json_object_get_string(value);
    return strlen(text) == (size_t)json_object_get_string_len(value) ? text : NULL;
}

static bool binding_equals(json_object *object, const char *key, const char *expected)
{
    const char *value = binding_string(object, key);
    return value && strcmp(value, expected) == 0;
}

static bool binding_true(json_object *object, const char *key)
{
    json_object *value = binding_child(object, key);
    return value && json_object_get_boolean(value);
}

static bool binding_identifier(const char *name)
{
    if (!name || !*name) return false;
    for (size_t i = 0; name[i]; i++) {
        unsigned char c = (unsigned char)name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ||
              (i && c >= '0' && c <= '9'))) return false;
    }
    return true;
}

static void binding_alias(json_object *node, const char *symbol)
{
    json_object_object_add(node, "c_alias", json_object_new_string(symbol));
    json_object_object_add(node, "has_c_alias", json_object_new_boolean(true));
}

static PackageNativeBinding *binding_find(PackageNativeBinding *bindings, size_t count,
                                         const char *name, const char *kind)
{
    if (!name) return NULL;
    for (size_t i = 0; i < count; i++)
        if (strcmp(bindings[i].name, name) == 0 && strcmp(bindings[i].kind, kind) == 0)
            return &bindings[i];
    return NULL;
}

static PackageNativeBinding *binding_method(PackageNativeBinding *bindings, size_t count,
                                           const char *owner, const char *method,
                                           const char *kind)
{
    if (!owner || !method) return NULL;
    size_t length = strlen(owner);
    for (size_t i = 0; i < count; i++)
        if (strncmp(bindings[i].name, owner, length) == 0 &&
            bindings[i].name[length] == '.' &&
            strcmp(bindings[i].name + length + 1, method) == 0 &&
            strcmp(bindings[i].kind, kind) == 0) return &bindings[i];
    return NULL;
}

static bool binding_declarations(json_object *model, PackageNativeBinding *bindings,
                                 size_t count)
{
    json_object *functions = binding_child(model, "functions");
    json_object *structures = binding_child(model, "structs");
    for (size_t i = 0; i < count; i++) {
        PackageNativeBinding *binding = &bindings[i];
        json_object *found = NULL;
        if (strcmp(binding->kind, "function") == 0) {
            for (size_t j = 0; j < json_object_array_length(functions); j++) {
                json_object *function = json_object_array_get_idx(functions, j);
                if (binding_equals(function, "name", binding->name)) { found = function; break; }
            }
        } else {
            for (size_t j = 0; j < json_object_array_length(structures); j++) {
                json_object *structure = json_object_array_get_idx(structures, j);
                json_object *methods = binding_child(structure, "methods");
                for (size_t k = 0; k < json_object_array_length(methods); k++) {
                    json_object *method = json_object_array_get_idx(methods, k);
                    const char *kind = binding_true(method, "is_static") ? "static" : "instance";
                    if (binding_method(binding, 1, binding_string(structure, "name"),
                                       binding_string(method, "name"), kind)) {
                        found = method;
                        if (!binding_string(found, "c_alias")) {
                            const char *owner = binding_string(structure, "name");
                            const char *name = binding_string(method, "name");
                            size_t size = strlen(owner) + strlen(name) + sizeof("__sn___");
                            binding->original = malloc(size);
                            if (!binding->original) return false;
                            snprintf(binding->original, size, "__sn__%s_%s", owner, name);
                        }
                        break;
                    }
                }
                if (found) break;
            }
        }
        if (!found || !binding_true(found, "is_native") || binding_true(found, "has_body")) {
            fprintf(stderr, "Error: package native binding has no matching native declaration: %s (%s)\n",
                    binding->name, binding->kind);
            return false;
        }
        if (!binding->original) {
            const char *original = binding_string(found, "c_alias");
            binding->original = strdup(original ? original : binding->name);
            if (!binding->original) return false;
        }
        binding_alias(found, binding->symbol);
    }
    return true;
}

static const char *binding_owner(json_object *object)
{
    json_object *type = binding_child(object, "type");
    if (binding_equals(type, "kind", "pointer")) type = binding_child(type, "base_type");
    return binding_string(type, "name");
}

static void binding_calls(json_object *node, PackageNativeBinding *bindings, size_t count)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            binding_calls(json_object_array_get_idx(node, i), bindings, count);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    PackageNativeBinding *binding = NULL;
    if (binding_equals(node, "kind", "call") && !binding_true(node, "is_closure_call") &&
        !binding_true(node, "is_fn_field_call")) {
        json_object *callee = binding_child(node, "callee");
        if (binding_true(binding_child(callee, "type"), "is_native")) {
            if (binding_equals(callee, "kind", "variable"))
                binding = binding_find(bindings, count, binding_string(callee, "name"), "function");
            else if (binding_equals(callee, "kind", "member"))
                binding = binding_method(bindings, count, binding_owner(binding_child(callee, "object")),
                                         binding_string(callee, "member_name"), "instance");
        }
        if (binding) binding_alias(callee, binding->symbol);
    } else if (binding_equals(node, "kind", "static_call")) {
        binding = binding_method(bindings, count, binding_string(node, "type_name"),
                                 binding_string(node, "method_name"), "static");
        if (binding) binding_alias(node, binding->symbol);
    }
    json_object_object_foreach(node, key, value) {
        (void)key;
        binding_calls(value, bindings, count);
    }
}

static bool binding_wrappers(json_object *model, PackageNativeBinding *bindings, size_t count)
{
    const char *keys[] = {"fn_wrappers", "threads"};
    for (size_t k = 0; k < sizeof(keys) / sizeof(keys[0]); k++) {
        json_object *wrappers = binding_child(model, keys[k]);
        for (size_t i = 0; i < json_object_array_length(wrappers); i++) {
            json_object *wrapper = json_object_array_get_idx(wrappers, i);
            if (!binding_true(wrapper, "is_native")) continue;
            const char *name = binding_string(wrapper, "target_name");
            /* Aliased targets retain their source identity separately from the
             * original C alias, which may itself name another public callable. */
            const char *logical = binding_string(wrapper, "func_name");
            PackageNativeBinding *found = binding_find(bindings, count, logical ? logical : name, "function");
            if (!found && name && !logical) {
                for (size_t j = 0; j < count; j++) {
                    if (strcmp(bindings[j].original, name) != 0) continue;
                    if (found) {
                        fprintf(stderr, "Error: ambiguous package native wrapper target: %s\n", name);
                        return false;
                    }
                    found = &bindings[j];
                }
            }
            if (found) json_object_object_add(wrapper, "target_name", json_object_new_string(found->symbol));
        }
    }
    return true;
}

bool gen_model_bind_package_natives(json_object *model, const char *filename)
{
    if (!filename) return true;
    json_object *input = json_object_from_file(filename);
    if (!input || !json_object_is_type(input, json_type_array)) {
        fprintf(stderr, "Error: package native bindings must be a JSON array: %s\n", filename);
        if (input) json_object_put(input);
        return false;
    }
    size_t count = json_object_array_length(input);
    PackageNativeBinding *bindings = calloc(count ? count : 1, sizeof(*bindings));
    bool valid = bindings != NULL;
    for (size_t i = 0; valid && i < count; i++) {
        json_object *entry = json_object_array_get_idx(input, i);
        bindings[i].name = binding_string(entry, "name");
        bindings[i].kind = binding_string(entry, "kind");
        bindings[i].symbol = binding_string(entry, "symbol");
        if (!bindings[i].name || !*bindings[i].name || !bindings[i].kind ||
            (strcmp(bindings[i].kind, "function") != 0 && strcmp(bindings[i].kind, "static") != 0 &&
             strcmp(bindings[i].kind, "instance") != 0) || !binding_identifier(bindings[i].symbol)) valid = false;
        for (size_t j = 0; valid && j < i; j++)
            if ((strcmp(bindings[j].name, bindings[i].name) == 0 &&
                 strcmp(bindings[j].kind, bindings[i].kind) == 0) ||
                strcmp(bindings[j].symbol, bindings[i].symbol) == 0) valid = false;
    }
    if (!valid) fprintf(stderr, "Error: invalid or duplicate package native callable binding\n");
    if (valid) valid = binding_declarations(model, bindings, count);
    if (valid) {
        binding_calls(model, bindings, count);
        valid = binding_wrappers(model, bindings, count);
    }
    if (bindings) for (size_t i = 0; i < count; i++) free(bindings[i].original);
    free(bindings);
    json_object_put(input);
    return valid;
}

/* The public zlib declarations use language uint references. The header's
 * uLongf can have a different width/effective type (notably LLP64 Windows).
 * Materialize typed C adapters without changing the imported declarations. */
typedef struct { const char *target; char *symbol; } HeaderReferenceAdapter;

static bool header_name_used(json_object *node, const char *name)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            if (header_name_used(json_object_array_get_idx(node, i), name)) return true;
    } else if (json_object_is_type(node, json_type_object)) {
        json_object_object_foreach(node, key, child) {
            if ((!strcmp(key, "name") || !strcmp(key, "c_alias") || !strcmp(key, "source_callable_name") ||
                 !strcmp(key, "target_name")) && json_object_is_type(child, json_type_string) &&
                !strcmp(json_object_get_string(child), name)) return true;
            if (header_name_used(child, name)) return true;
        }
    }
    return false;
}

static bool header_zlib_signature(json_object *function, const char *target)
{
    if (!binding_true(function, "is_native") || binding_true(function, "has_body") ||
        binding_true(function, "is_variadic") ||
        !binding_equals(binding_child(function, "return_type"), "kind", "int")) return false;
    json_object *params = binding_child(function, "params");
    size_t count = !strcmp(target, "compress2") ? 5 : 4;
    if (json_object_array_length(params) != count) return false;
    for (size_t i = 0; i < count; i++) {
        json_object *param = json_object_array_get_idx(params, i), *type = binding_child(param, "type");
        const char *qualifier = binding_string(param, "mem_qual");
        if (i == 0 || i == 2) {
            if (!binding_equals(type, "kind", "pointer") ||
                !binding_equals(binding_child(type, "base_type"), "kind", "byte")) return false;
        } else if (!binding_equals(type, "kind", i == 4 ? "int" : "uint")) return false;
        if (i == 1 ? (!qualifier || strcmp(qualifier, "as_ref")) :
            (qualifier && strcmp(qualifier, "default"))) return false;
        if (binding_string(param, "sync_mod") && !binding_equals(param, "sync_mod", "none")) return false;
    }
    return true;
}

static char *header_zlib_body(json_object *function, const char *target)
{
    json_object *params = binding_child(function, "params");
    const char *names[5] = {0}; size_t capacity = 2048;
    for (size_t i = 0; i < json_object_array_length(params); i++) {
        names[i] = binding_string(json_object_array_get_idx(params, i), "name");
        if (!binding_identifier(names[i])) return NULL;
        capacity += strlen(names[i]) * 16;
    }
    char *code = malloc(capacity), *direct = malloc(capacity), *converted = malloc(capacity);
    if (!code || !direct || !converted) { free(code); free(direct); free(converted); return NULL; }
    const char *level = names[4];
    snprintf(direct, capacity, "%s(__sn__%s, (uLongf *)(void *)__sn__%s, __sn__%s, (uLong)__sn__%s%s%s)",
        target, names[0], names[1], names[2], names[3], level ? ", (int)__sn__" : "", level ? level : "");
    snprintf(converted, capacity, "%s(__sn__%s, &sn_header_length, __sn__%s, (uLong)__sn__%s%s%s)",
        target, names[0], names[2], names[3], level ? ", (int)__sn__" : "", level ? level : "");
    snprintf(code, capacity,
        "if (_Generic((uint64_t *)0, uLongf *: 1, default: 0)) { return (long long)%s; }\n"
        "uLongf sn_header_length = (uLongf)*__sn__%s;\n"
        "int sn_header_result = %s;\n"
        "*__sn__%s = (uint64_t)sn_header_length;\nreturn (long long)sn_header_result;",
        direct, names[1], converted, names[1]);
    free(direct); free(converted); return code;
}

static void header_rebind_uses(json_object *node, HeaderReferenceAdapter *adapters, size_t count)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            header_rebind_uses(json_object_array_get_idx(node, i), adapters, count);
    } else if (json_object_is_type(node, json_type_object)) {
        if ((binding_equals(node, "kind", "variable") || binding_equals(node, "kind", "member")) &&
            binding_true(binding_child(node, "type"), "is_native")) {
            for (size_t i = 0; i < count; i++)
                if (adapters[i].symbol && binding_equals(node, "c_alias", adapters[i].target)) binding_alias(node, adapters[i].symbol);
        }
        if (binding_true(node, "is_native")) {
            for (size_t i = 0; i < count; i++)
                if (adapters[i].symbol && binding_equals(node, "target_name", adapters[i].target))
                    json_object_object_add(node, "target_name", json_object_new_string(adapters[i].symbol));
        }
        json_object_object_foreach(node, key, child) {
            (void)key; header_rebind_uses(child, adapters, count);
        }
    }
}

/* Namespace imports omit header-only extern declarations in the C model.
 * Their checked callable types still carry the complete reference signature. */
static void header_collect_callables(json_object *node, json_object *candidates)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            header_collect_callables(json_object_array_get_idx(node, i), candidates);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object *type = binding_child(node, "type");
    const char *alias = binding_string(node, "c_alias");
    if (alias && (!strcmp(alias, "compress") || !strcmp(alias, "compress2") || !strcmp(alias, "uncompress")) &&
        binding_equals(type, "kind", "function") && binding_true(type, "is_native")) {
        json_object *function = json_object_new_object(), *params = json_object_new_array();
        json_object_object_add(function, "name", json_object_new_string(alias));
        binding_alias(function, alias);
        json_object_object_add(function, "is_native", json_object_new_boolean(true));
        json_object_object_add(function, "return_type", json_object_get(binding_child(type, "return_type")));
        json_object_object_add(function, "is_variadic", json_object_new_boolean(binding_true(type, "is_variadic")));
        json_object *types = binding_child(type, "param_types"), *quals = binding_child(type, "param_mem_quals");
        for (size_t i = 0; i < json_object_array_length(types); i++) {
            json_object *param = json_object_new_object(); char name[32];
            snprintf(name, sizeof(name), "p%zu", i);
            json_object_object_add(param, "name", json_object_new_string(name));
            json_object_object_add(param, "type", json_object_get(json_object_array_get_idx(types, i)));
            json_object *qualifier = json_object_array_get_idx(quals, i);
            json_object_object_add(param, "mem_qual", qualifier ? json_object_get(qualifier) : json_object_new_string("default"));
            json_object_array_add(params, param);
        }
        json_object_object_add(function, "params", params);
        json_object_array_add(candidates, function);
    }
    json_object_object_foreach(node, key, child) {
        (void)key; header_collect_callables(child, candidates);
    }
}

bool gen_model_adapt_header_references(json_object *model)
{
    json_object *pragmas = binding_child(model, "pragmas"), *functions = binding_child(model, "functions");
    bool zlib = false;
    for (size_t i = 0; i < json_object_array_length(pragmas); i++) {
        json_object *pragma = json_object_array_get_idx(pragmas, i);
        if (binding_equals(pragma, "pragma_type", "include") &&
            (binding_equals(pragma, "value", "<zlib.h>") || binding_equals(pragma, "value", "\"zlib.h\""))) zlib = true;
    }
    if (!zlib) return true;
    HeaderReferenceAdapter adapters[3] = {{"compress2", NULL}, {"compress", NULL}, {"uncompress", NULL}};
    json_object *bodies = json_object_new_array(), *candidates = json_object_new_array();
    bool valid = bodies && candidates;
    for (size_t i = 0; valid && i < json_object_array_length(functions); i++)
        json_object_array_add(candidates, json_object_get(json_object_array_get_idx(functions, i)));
    if (valid) header_collect_callables(model, candidates);
    size_t count = json_object_array_length(candidates);
    for (size_t i = 0; valid && i < count; i++) {
        json_object *function = json_object_array_get_idx(candidates, i);
        for (size_t j = 0; valid && j < 3; j++) {
            HeaderReferenceAdapter *adapter = &adapters[j];
            if (!binding_equals(function, "c_alias", adapter->target) || !header_zlib_signature(function, adapter->target)) continue;
            if (!adapter->symbol) {
                char name[128]; unsigned index = 0;
                do { snprintf(name, sizeof(name), "__sn_header_zlib_%s_%u", adapter->target, index++); }
                while (header_name_used(model, name));
                adapter->symbol = strdup(name);
                json_object *clone = NULL;
                char *code = header_zlib_body(function, adapter->target);
                valid = adapter->symbol && code && json_object_deep_copy(function, &clone, NULL) == 0;
                if (!valid) { free(code); if (clone) json_object_put(clone); break; }
                json_object_object_add(clone, "name", json_object_new_string(adapter->symbol));
                json_object_object_del(clone, "c_alias"); json_object_object_del(clone, "has_c_alias");
                json_object_object_del(clone, "source_callable_name");
                json_object_object_del(clone, "has_pragma_source");
                json_object_object_add(clone, "has_body", json_object_new_boolean(true));
                json_object *body = json_object_new_array(), *stmt = json_object_new_object();
                json_object_object_add(stmt, "kind", json_object_new_string("raw_c"));
                json_object_object_add(stmt, "code", json_object_new_string(code)); free(code);
                json_object_array_add(body, stmt);json_object_object_add(clone, "body", body);
                json_object_array_add(bodies, clone);
            }
            binding_alias(function, adapter->symbol);
        }
    }
    if (valid) {
        header_rebind_uses(model, adapters, 3);
        for (size_t i = 0; i < json_object_array_length(bodies); i++)
            json_object_array_add(functions, json_object_get(json_object_array_get_idx(bodies, i)));
    } else fprintf(stderr, "Error: cannot generate typed zlib header reference adapters\n");
    for (size_t i = 0; i < 3; i++) free(adapters[i].symbol);
    if (bodies) json_object_put(bodies);
    if (candidates) json_object_put(candidates);
    return valid;
}
