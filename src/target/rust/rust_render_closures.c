/* Included by rust_render.c. Function types remain rejected by validation;
 * retain the previous renderer fallback until that family is implemented. */
static char *rust_closure_type(json_object *type)
{
    (void)type;
    return strdup("()");
}
