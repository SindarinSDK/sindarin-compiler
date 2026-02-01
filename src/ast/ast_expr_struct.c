// ast_expr_struct.c
// Struct, member, sizeof expressions and escape analysis functions

Expr *ast_create_struct_literal_expr(Arena *arena, Token struct_name, FieldInitializer *fields,
                                      int field_count, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_STRUCT_LITERAL;

    /* Clone struct_name token */
    char *name_start = arena_strndup(arena, struct_name.start, struct_name.length);
    if (name_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.struct_literal.struct_name.start = name_start;
    expr->as.struct_literal.struct_name.length = struct_name.length;
    expr->as.struct_literal.struct_name.line = struct_name.line;
    expr->as.struct_literal.struct_name.type = struct_name.type;
    expr->as.struct_literal.struct_name.filename = struct_name.filename;

    expr->as.struct_literal.field_count = field_count;
    expr->as.struct_literal.struct_type = NULL;  /* Set during type checking */
    expr->as.struct_literal.fields_initialized = NULL;  /* Allocated during type checking */
    expr->as.struct_literal.total_field_count = 0;  /* Set during type checking */

    if (field_count > 0 && fields != NULL)
    {
        expr->as.struct_literal.fields = arena_alloc(arena, sizeof(FieldInitializer) * field_count);
        if (expr->as.struct_literal.fields == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        for (int i = 0; i < field_count; i++)
        {
            /* Clone field name token */
            char *field_start = arena_strndup(arena, fields[i].name.start, fields[i].name.length);
            if (field_start == NULL)
            {
                DEBUG_ERROR("Out of memory");
                exit(1);
            }
            expr->as.struct_literal.fields[i].name.start = field_start;
            expr->as.struct_literal.fields[i].name.length = fields[i].name.length;
            expr->as.struct_literal.fields[i].name.line = fields[i].name.line;
            expr->as.struct_literal.fields[i].name.type = fields[i].name.type;
            expr->as.struct_literal.fields[i].name.filename = fields[i].name.filename;
            expr->as.struct_literal.fields[i].value = fields[i].value;
        }
    }
    else
    {
        expr->as.struct_literal.fields = NULL;
    }

    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_member_access_expr(Arena *arena, Expr *object, Token field_name, const Token *loc_token)
{
    if (object == NULL)
    {
        DEBUG_ERROR("Cannot create member access with NULL object");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_MEMBER_ACCESS;
    expr->as.member_access.object = object;

    /* Clone field_name token */
    char *name_start = arena_strndup(arena, field_name.start, field_name.length);
    if (name_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.member_access.field_name.start = name_start;
    expr->as.member_access.field_name.length = field_name.length;
    expr->as.member_access.field_name.line = field_name.line;
    expr->as.member_access.field_name.type = field_name.type;
    expr->as.member_access.field_name.filename = field_name.filename;
    expr->as.member_access.field_index = -1;  /* Set during type checking */
    expr->as.member_access.escaped = false;   /* Set during escape analysis */
    expr->as.member_access.scope_depth = 0;   /* Set during type checking */

    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_member_assign_expr(Arena *arena, Expr *object, Token field_name, Expr *value, const Token *loc_token)
{
    if (object == NULL || value == NULL)
    {
        DEBUG_ERROR("Cannot create member assign with NULL object or value");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_MEMBER_ASSIGN;
    expr->as.member_assign.object = object;
    expr->as.member_assign.value = value;

    /* Clone field_name token */
    char *name_start = arena_strndup(arena, field_name.start, field_name.length);
    if (name_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.member_assign.field_name.start = name_start;
    expr->as.member_assign.field_name.length = field_name.length;
    expr->as.member_assign.field_name.line = field_name.line;
    expr->as.member_assign.field_name.type = field_name.type;
    expr->as.member_assign.field_name.filename = field_name.filename;

    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_sizeof_type_expr(Arena *arena, Type *type_operand, const Token *loc_token)
{
    if (type_operand == NULL)
    {
        DEBUG_ERROR("Cannot create sizeof expression with NULL type");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_SIZEOF;
    expr->as.sizeof_expr.type_operand = type_operand;
    expr->as.sizeof_expr.expr_operand = NULL;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_sizeof_expr_expr(Arena *arena, Expr *expr_operand, const Token *loc_token)
{
    if (expr_operand == NULL)
    {
        DEBUG_ERROR("Cannot create sizeof expression with NULL expression");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_SIZEOF;
    expr->as.sizeof_expr.type_operand = NULL;
    expr->as.sizeof_expr.expr_operand = expr_operand;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

/* ============================================================================
 * Escape Analysis Helper Functions
 * ============================================================================ */

void ast_expr_mark_escapes(Expr *expr)
{
    if (expr != NULL)
    {
        expr->escape_info.escapes_scope = true;
    }
}

void ast_expr_mark_needs_heap(Expr *expr)
{
    if (expr != NULL)
    {
        expr->escape_info.needs_heap_allocation = true;
    }
}

void ast_expr_clear_escape_info(Expr *expr)
{
    if (expr != NULL)
    {
        expr->escape_info.escapes_scope = false;
        expr->escape_info.needs_heap_allocation = false;
    }
}

bool ast_expr_escapes_scope(Expr *expr)
{
    if (expr == NULL)
    {
        return false;
    }
    return expr->escape_info.escapes_scope;
}

bool ast_expr_needs_heap_allocation(Expr *expr)
{
    if (expr == NULL)
    {
        return false;
    }
    return expr->escape_info.needs_heap_allocation;
}
