#ifndef SN_RUST_RECORD_PARAMETER_GUARDS_H
#define SN_RUST_RECORD_PARAMETER_GUARDS_H

#include <json-c/json.h>
#include <string.h>

static void rust_collect_record_parameter_guards(json_object *used, json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_collect_record_parameter_guards(used, json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    if (json_object_get_boolean(json_object_object_get(node, "rust_native_record_parameter_rebind")))
    {
        json_object *type = NULL;
        json_object_object_get_ex(node, "type", &type);
        const char *name = json_object_get_string(json_object_object_get(type, "name"));
        if (name) json_object_object_add(used, name, json_object_new_boolean(true));
    }
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0) rust_collect_record_parameter_guards(used, child);
}

static void rust_mark_record_parameter_guards(json_object *model)
{
    json_object *structs = json_object_object_get(model, "structs");
    /* Struct/type nodes are shared in the model. Annotating a structure from
     * a descendant can resize the very object an ancestor is iterating over.
     * Collect names first, then mutate only after the traversal has ended. */
    json_object *used = json_object_new_object();
    rust_collect_record_parameter_guards(used, model);
    for (size_t i = 0; structs && i < json_object_array_length(structs); i++)
    {
        json_object *structure = json_object_array_get_idx(structs, i), *guard = NULL;
        const char *name = json_object_get_string(json_object_object_get(structure, "name"));
        if (name && json_object_object_get_ex(used, name, &guard))
            json_object_object_add(structure, "rust_native_record_parameter_guard_used", json_object_new_boolean(true));
    }
    json_object_put(used);
}

#endif
