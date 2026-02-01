// ast_expr_basic.c
// Basic expression creation functions

Expr *ast_create_comparison_expr(Arena *arena, Expr *left, Expr *right, SnTokenType comparison_type, const Token *loc_token)
{
    if (left == NULL || right == NULL)
    {
        DEBUG_ERROR("Cannot create comparison with NULL expressions");
        return NULL;
    }

    return ast_create_binary_expr(arena, left, comparison_type, right, loc_token);
}

Expr *ast_create_binary_expr(Arena *arena, Expr *left, SnTokenType operator, Expr *right, const Token *loc_token)
{
    if (left == NULL || right == NULL)
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
    expr->type = EXPR_BINARY;
    expr->as.binary.left = left;
    expr->as.binary.right = right;
    expr->as.binary.operator = operator;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_unary_expr(Arena *arena, SnTokenType operator, Expr *operand, const Token *loc_token)
{
    if (operand == NULL)
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
    expr->type = EXPR_UNARY;
    expr->as.unary.operator = operator;
    expr->as.unary.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_literal_expr(Arena *arena, LiteralValue value, Type *type, bool is_interpolated, const Token *loc_token)
{
    if (type == NULL)
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
    expr->type = EXPR_LITERAL;
    expr->as.literal.value = value;
    expr->as.literal.type = type;
    expr->as.literal.is_interpolated = is_interpolated;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_variable_expr(Arena *arena, Token name, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_VARIABLE;
    char *new_start = arena_strndup(arena, name.start, name.length);
    if (new_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.variable.name.start = new_start;
    expr->as.variable.name.length = name.length;
    expr->as.variable.name.line = name.line;
    expr->as.variable.name.type = name.type;
    expr->as.variable.name.filename = name.filename;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_assign_expr(Arena *arena, Token name, Expr *value, const Token *loc_token)
{
    if (value == NULL)
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
    expr->type = EXPR_ASSIGN;
    char *new_start = arena_strndup(arena, name.start, name.length);
    if (new_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.assign.name.start = new_start;
    expr->as.assign.name.length = name.length;
    expr->as.assign.name.line = name.line;
    expr->as.assign.name.type = name.type;
    expr->as.assign.name.filename = name.filename;
    expr->as.assign.value = value;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_index_assign_expr(Arena *arena, Expr *array, Expr *index, Expr *value, const Token *loc_token)
{
    if (array == NULL || index == NULL || value == NULL)
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
    expr->type = EXPR_INDEX_ASSIGN;
    expr->as.index_assign.array = array;
    expr->as.index_assign.index = index;
    expr->as.index_assign.value = value;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_increment_expr(Arena *arena, Expr *operand, const Token *loc_token)
{
    if (operand == NULL)
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
    expr->type = EXPR_INCREMENT;
    expr->as.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_decrement_expr(Arena *arena, Expr *operand, const Token *loc_token)
{
    if (operand == NULL)
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
    expr->type = EXPR_DECREMENT;
    expr->as.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_compound_assign_expr(Arena *arena, Expr *target, SnTokenType operator, Expr *value, const Token *loc_token)
{
    if (target == NULL || value == NULL)
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
    expr->type = EXPR_COMPOUND_ASSIGN;
    expr->as.compound_assign.target = target;
    expr->as.compound_assign.operator = operator;
    expr->as.compound_assign.value = value;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}
