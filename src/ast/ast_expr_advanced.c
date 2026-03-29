// ast_expr_advanced.c
// Advanced expression creation functions (lambda, thread, type expressions)

Expr *ast_create_lambda_expr(Arena *arena, Parameter *params, int param_count,
                             Type *return_type, Expr *body, FunctionModifier modifier,
                             bool is_native, const Token *loc_token)
{
    /* body is required, but return_type can be NULL for type inference */
    if (body == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_LAMBDA;
    expr->as.lambda.params = params;
    expr->as.lambda.param_count = param_count;
    expr->as.lambda.return_type = return_type;
    expr->as.lambda.body = body;
    expr->as.lambda.body_stmts = NULL;
    expr->as.lambda.body_stmt_count = 0;
    expr->as.lambda.has_stmt_body = 0;  /* Expression body */
    expr->as.lambda.modifier = modifier;
    expr->as.lambda.is_native = is_native;
    expr->as.lambda.captured_vars = NULL;
    expr->as.lambda.captured_types = NULL;
    expr->as.lambda.capture_count = 0;
    expr->as.lambda.lambda_id = 0;  /* Assigned during code gen */
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_lambda_stmt_expr(Arena *arena, Parameter *params, int param_count,
                                  Type *return_type, Stmt **body_stmts, int body_stmt_count,
                                  FunctionModifier modifier, bool is_native, const Token *loc_token)
{
    /* body_stmts is required for statement-body lambdas */
    if (body_stmts == NULL && body_stmt_count > 0)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_LAMBDA;
    expr->as.lambda.params = params;
    expr->as.lambda.param_count = param_count;
    expr->as.lambda.return_type = return_type;
    expr->as.lambda.body = NULL;
    expr->as.lambda.body_stmts = body_stmts;
    expr->as.lambda.body_stmt_count = body_stmt_count;
    expr->as.lambda.has_stmt_body = 1;  /* Statement body */
    expr->as.lambda.modifier = modifier;
    expr->as.lambda.is_native = is_native;
    expr->as.lambda.captured_vars = NULL;
    expr->as.lambda.captured_types = NULL;
    expr->as.lambda.capture_count = 0;
    expr->as.lambda.lambda_id = 0;  /* Assigned during code gen */
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_thread_spawn_expr(Arena *arena, Expr *call, FunctionModifier modifier, const Token *loc_token)
{
    if (call == NULL)
    {
        DEBUG_ERROR("Cannot create thread spawn with NULL call expression");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_THREAD_SPAWN;
    expr->as.thread_spawn.call = call;
    expr->as.thread_spawn.modifier = modifier;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_thread_sync_expr(Arena *arena, Expr *handle, bool is_array, const Token *loc_token)
{
    if (handle == NULL)
    {
        DEBUG_ERROR("Cannot create thread sync with NULL handle expression");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_THREAD_SYNC;
    expr->as.thread_sync.handle = handle;
    expr->as.thread_sync.is_array = is_array;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_thread_detach_expr(Arena *arena, Expr *handle, const Token *loc_token)
{
    if (handle == NULL)
    {
        DEBUG_ERROR("Cannot create thread detach with NULL handle expression");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_THREAD_DETACH;
    expr->as.thread_detach.handle = handle;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_sync_list_expr(Arena *arena, Expr **elements, int element_count, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_SYNC_LIST;
    expr->as.sync_list.elements = elements;
    expr->as.sync_list.element_count = element_count;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_address_of_expr(Arena *arena, Expr *operand, const Token *loc_token)
{
    if (operand == NULL)
    {
        DEBUG_ERROR("Cannot create addressOf expression with NULL operand");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_ADDRESS_OF;
    expr->as.address_of.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_value_of_expr(Arena *arena, Expr *operand, const Token *loc_token)
{
    if (operand == NULL)
    {
        DEBUG_ERROR("Cannot create valueOf expression with NULL operand");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_VALUE_OF;
    expr->as.value_of.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_copy_of_expr(Arena *arena, Expr *operand, const Token *loc_token)
{
    if (operand == NULL)
    {
        DEBUG_ERROR("Cannot create copyOf expression with NULL operand");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_COPY_OF;
    expr->as.copy_of.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_typeof_expr(Arena *arena, Expr *operand, const Token *loc_token)
{
    if (operand == NULL)
    {
        DEBUG_ERROR("Cannot create typeOf expression with NULL operand");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_TYPEOF;
    expr->as.typeof_expr.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_match_expr(Arena *arena, Expr *subject, MatchArm *arms, int arm_count, const Token *loc_token)
{
    if (subject == NULL)
    {
        DEBUG_ERROR("Cannot create match expression with NULL subject");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_MATCH;
    expr->as.match_expr.subject = subject;
    expr->as.match_expr.arms = arms;
    expr->as.match_expr.arm_count = arm_count;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}
