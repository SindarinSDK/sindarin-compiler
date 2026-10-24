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
            PackageNativeBinding *found = binding_find(bindings, count, name, "function");
            if (!found && name) {
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
