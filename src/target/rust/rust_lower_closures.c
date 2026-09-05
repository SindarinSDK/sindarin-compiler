/* Included by rust_lower.c. Invoked only after full model validation, before
 * existing lowering passes. The blanket closure gate makes this a no-op until
 * the closure family supplies its target-local lowering. */
static void rust_lower_closures(json_object *model)
{
    (void)model;
}
