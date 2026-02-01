/* ==============================================================================
 * code_gen_expr_lambda_native.c - Native Lambda Generation
 * ==============================================================================
 * Code generation for native lambdas (C-compatible function pointers).
 * ============================================================================== */

/* Generate code for a native lambda expression (C-compatible function pointer).
 * Native lambdas don't use closures - they're direct function pointers. */
static char *code_gen_native_lambda_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_native_lambda_expression");
    LambdaExpr *lambda = &expr->as.lambda;
    int lambda_id = gen->lambda_count++;

    /* Store the lambda_id in the expression for later reference */
    expr->as.lambda.lambda_id = lambda_id;

    /* Get C types for return type and parameters */
    const char *ret_c_type = get_c_type(gen->arena, lambda->return_type);

    /* Build parameter list string for the static function (no closure param) */
    char *params_decl = arena_strdup(gen->arena, "");
    for (int i = 0; i < lambda->param_count; i++)
    {
        const char *param_c_type = get_c_type(gen->arena, lambda->params[i].type);
        char *param_name = sn_mangle_name(gen->arena, arena_strndup(gen->arena, lambda->params[i].name.start,
                                         lambda->params[i].name.length));
        if (i > 0)
        {
            params_decl = arena_sprintf(gen->arena, "%s, %s %s", params_decl, param_c_type, param_name);
        }
        else
        {
            params_decl = arena_sprintf(gen->arena, "%s %s", param_c_type, param_name);
        }
    }

    /* Handle empty parameter list */
    if (lambda->param_count == 0)
    {
        params_decl = arena_strdup(gen->arena, "void");
    }

    /* Generate the lambda function name */
    char *lambda_func_name = arena_sprintf(gen->arena, "__lambda_%d__", lambda_id);

    /* Generate forward declaration */
    gen->lambda_forward_decls = arena_sprintf(gen->arena, "%sstatic %s %s(%s);\n",
                                              gen->lambda_forward_decls,
                                              ret_c_type, lambda_func_name, params_decl);

    /* Generate the lambda function body */
    char *lambda_func;
    if (lambda->has_stmt_body)
    {
        /* Statement body lambda - use helper to generate body */
        char *body_code = code_gen_lambda_stmt_body(gen, lambda, 1, lambda_func_name, lambda->return_type);

        /* Generate the function with statement body */
        lambda_func = arena_sprintf(gen->arena,
            "static %s %s(%s) {\n"
            "    %s _return_value = %s;\n"
            "%s"
            "%s_return:\n"
            "    return _return_value;\n"
            "}\n\n",
            ret_c_type, lambda_func_name, params_decl,
            ret_c_type, get_default_value(lambda->return_type),
            body_code,
            lambda_func_name);
    }
    else
    {
        /* Expression body lambda */
        char *body_code = code_gen_expression(gen, lambda->body);

        /* Generate simple function body */
        lambda_func = arena_sprintf(gen->arena,
            "static %s %s(%s) {\n"
            "    return %s;\n"
            "}\n\n",
            ret_c_type, lambda_func_name, params_decl, body_code);
    }

    /* Append to definitions buffer */
    gen->lambda_definitions = arena_sprintf(gen->arena, "%s%s",
                                            gen->lambda_definitions, lambda_func);

    /* Return the function pointer directly (no closure) */
    return arena_sprintf(gen->arena, "__lambda_%d__", lambda_id);
}
