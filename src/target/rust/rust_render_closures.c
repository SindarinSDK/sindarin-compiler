/* A signature is represented by Rust's arity-specific Fn trait. Rc owns the
 * callable identity; cloning a handle never clones its environment. */
static char *rust_closure_type(json_object *type)
{
    json_object *params = NULL, *ret = NULL;
    json_object_object_get_ex(type, "param_types", &params);
    json_object_object_get_ex(type, "return_type", &ret);
    char *result = strdup("__SnClosure<dyn Fn(");
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
    char *grown = realloc(result, length + strlen(part) + 7);
    if (!grown) { free(part); free(result); return NULL; }
    result = grown;
    sprintf(result + length, ") -> %s>", part);
    free(part);
    return result;
}
