// ast_expr_call.c
// Call, array, and member expression creation functions

Expr *ast_create_call_expr(Arena *arena, Expr *callee, Expr **arguments, int arg_count, const Token *loc_token)
{
    if (callee == NULL)
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
    expr->type = EXPR_CALL;
    expr->as.call.callee = callee;
    expr->as.call.arguments = arguments;
    expr->as.call.arg_count = arg_count;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_array_expr(Arena *arena, Expr **elements, int element_count, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_ARRAY;
    expr->as.array.elements = elements;
    expr->as.array.element_count = element_count;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_array_access_expr(Arena *arena, Expr *array, Expr *index, const Token *loc_token)
{
    if (array == NULL || index == NULL)
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
    expr->type = EXPR_ARRAY_ACCESS;
    expr->as.array_access.array = array;
    expr->as.array_access.index = index;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_sized_array_alloc_expr(Arena *arena, Type *element_type, Expr *size_expr, Expr *default_value, const Token *loc_token)
{
    if (element_type == NULL || size_expr == NULL)
    {
        DEBUG_ERROR("Cannot create sized array alloc with NULL element_type or size_expr");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_SIZED_ARRAY_ALLOC;
    expr->as.sized_array_alloc.element_type = element_type;
    expr->as.sized_array_alloc.size_expr = size_expr;
    expr->as.sized_array_alloc.default_value = default_value;  /* Can be NULL */
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_interpolated_expr(Arena *arena, Expr **parts, char **format_specs, int part_count, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_INTERPOLATED;
    expr->as.interpol.parts = parts;
    expr->as.interpol.format_specs = format_specs;
    expr->as.interpol.part_count = part_count;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_member_expr(Arena *arena, Expr *object, Token member_name, const Token *loc_token)
{
    if (object == NULL)
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
    expr->type = EXPR_MEMBER;
    expr->as.member.object = object;
    char *new_start = arena_strndup(arena, member_name.start, member_name.length);
    if (new_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.member.member_name.start = new_start;
    expr->as.member.member_name.length = member_name.length;
    expr->as.member.member_name.line = member_name.line;
    expr->as.member.member_name.type = member_name.type;
    expr->as.member.member_name.filename = member_name.filename;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_array_slice_expr(Arena *arena, Expr *array, Expr *start, Expr *end, Expr *step, const Token *loc_token)
{
    if (array == NULL)
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
    expr->type = EXPR_ARRAY_SLICE;
    expr->as.array_slice.array = array;
    expr->as.array_slice.start = start;  // Can be NULL
    expr->as.array_slice.end = end;      // Can be NULL
    expr->as.array_slice.step = step;    // Can be NULL (defaults to 1)
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_range_expr(Arena *arena, Expr *start, Expr *end, const Token *loc_token)
{
    if (start == NULL || end == NULL)
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
    expr->type = EXPR_RANGE;
    expr->as.range.start = start;
    expr->as.range.end = end;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_spread_expr(Arena *arena, Expr *array, const Token *loc_token)
{
    if (array == NULL)
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
    expr->type = EXPR_SPREAD;
    expr->as.spread.array = array;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_static_call_expr(Arena *arena, Token type_name, Token method_name,
                                   Expr **arguments, int arg_count, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_STATIC_CALL;

    // Clone type_name token
    char *type_start = arena_strndup(arena, type_name.start, type_name.length);
    if (type_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.static_call.type_name.start = type_start;
    expr->as.static_call.type_name.length = type_name.length;
    expr->as.static_call.type_name.line = type_name.line;
    expr->as.static_call.type_name.type = type_name.type;
    expr->as.static_call.type_name.filename = type_name.filename;

    // Clone method_name token
    char *method_start = arena_strndup(arena, method_name.start, method_name.length);
    if (method_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.static_call.method_name.start = method_start;
    expr->as.static_call.method_name.length = method_name.length;
    expr->as.static_call.method_name.line = method_name.line;
    expr->as.static_call.method_name.type = method_name.type;
    expr->as.static_call.method_name.filename = method_name.filename;

    expr->as.static_call.arguments = arguments;
    expr->as.static_call.arg_count = arg_count;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}
