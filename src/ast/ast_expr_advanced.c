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

Expr *ast_create_as_val_expr(Arena *arena, Expr *operand, const Token *loc_token)
{
    if (operand == NULL)
    {
        DEBUG_ERROR("Cannot create as_val expression with NULL operand");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_AS_VAL;
    expr->as.as_val.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_as_ref_expr(Arena *arena, Expr *operand, const Token *loc_token)
{
    if (operand == NULL)
    {
        DEBUG_ERROR("Cannot create as_ref expression with NULL operand");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_AS_REF;
    expr->as.as_ref.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_typeof_expr(Arena *arena, Expr *operand, Type *type_literal, const Token *loc_token)
{
    /* Either operand or type_literal must be non-NULL, but not both */
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_TYPEOF;
    expr->as.typeof_expr.operand = operand;
    expr->as.typeof_expr.type_literal = type_literal;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_is_expr(Arena *arena, Expr *operand, Type *check_type, const Token *loc_token)
{
    if (operand == NULL || check_type == NULL)
    {
        DEBUG_ERROR("Cannot create is expression with NULL operand or type");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_IS;
    expr->as.is_expr.operand = operand;
    expr->as.is_expr.check_type = check_type;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_as_type_expr(Arena *arena, Expr *operand, Type *target_type, const Token *loc_token)
{
    if (operand == NULL || target_type == NULL)
    {
        DEBUG_ERROR("Cannot create as type expression with NULL operand or type");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_AS_TYPE;
    expr->as.as_type.operand = operand;
    expr->as.as_type.target_type = target_type;
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
