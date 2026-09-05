/* Included by rust_validate.c. Keep the model-wide gate before threads,
 * declarations, structs and callable validation. No closure representation is
 * selected here; all hooks preserve the old gates/fallbacks exactly. */
static bool rust_validate_closures(json_object *model)
{
    return array_is_empty(model, "lambdas");
}

static bool rust_closure_type_supported(json_object *type)
{
    (void)type;
    return false;
}

static bool rust_validate_lambda(json_object *expr)
{
    (void)expr;
    return false;
}

/* Variable expressions were accepted without inspecting their function type.
 * Storage/signature validation and the model-wide lambda gate still apply. */
static bool rust_validate_function_value(json_object *expr)
{
    (void)expr;
    return true;
}

/* Called before ordinary callee/argument validation. UNHANDLED means continue
 * the existing call path, including its diagnostics and child order. */
static RustValidationResult rust_validate_closure_call(json_object *expr)
{
    (void)expr;
    return RUST_VALIDATION_UNHANDLED;
}
