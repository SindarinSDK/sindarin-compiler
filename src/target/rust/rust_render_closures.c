/* A signature is represented by Rust's arity-specific Fn trait. Rc owns the
 * callable identity; cloning a handle never clones its environment. */
static char *rust_closure_type(json_object *type)
{
    json_object *params = NULL, *ret = NULL;
    json_object_object_get_ex(type, "param_types", &params);
    json_object_object_get_ex(type, "return_type", &ret);
    const char *name = json_string_property(type, "rust_closure_handle_name");
    if (!name) name = "__SnClosure";
    char *result = malloc(strlen(name) + sizeof("<dyn Fn("));
    if (!result) return NULL;
    sprintf(result, "%s<dyn Fn(", name);
    size_t length = strlen(result);
    for (size_t i = 0; i < json_object_array_length(params); i++)
    {
        char *part = rust_type(json_object_array_get_idx(params, i));
        size_t extra = strlen(part) + (i ? 2 : 0);
        char *grown = realloc(result, length + extra + 1);
        if (!grown) { free(part); free(result); return NULL; }
        result = grown;
        if (i) { memcpy(result + length, ", ", 2); length += 2; }
        strcpy(result + length, part);
        length += strlen(part);
        free(part);
    }
    char *part = rust_type(ret);
    json_object *thread_ownership = NULL;
    const char *bounds = json_object_object_get_ex(type, "rust_thread_ownership", &thread_ownership) &&
        json_object_get_boolean(thread_ownership) ? " + Send + Sync" : "";
    char *grown = realloc(result, length + strlen(part) + strlen(bounds) + 7);
    if (!grown) { free(part); free(result); return NULL; }
    result = grown;
    sprintf(result + length, ") -> %s%s>", part, bounds);
    free(part);
    return result;
}
