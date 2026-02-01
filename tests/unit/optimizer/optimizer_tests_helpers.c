// optimizer_tests_helpers.c
// Helper functions for optimizer tests

/* Helper to create an int literal expression */
static Expr *create_int_literal(Arena *arena, int64_t value)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    expr->type = EXPR_LITERAL;
    expr->as.literal.value.int_value = value;
    expr->as.literal.type = ast_create_primitive_type(arena, TYPE_INT);
    expr->expr_type = expr->as.literal.type;
    return expr;
}

/* Helper to create a variable expression */
static Expr *create_variable_expr(Arena *arena, const char *name)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    expr->type = EXPR_VARIABLE;
    setup_basic_token(&expr->as.variable.name, TOKEN_IDENTIFIER, name);
    expr->expr_type = ast_create_primitive_type(arena, TYPE_INT);
    return expr;
}

/* Helper to create a binary expression */
static Expr *create_binary_expr(Arena *arena, Expr *left, SnTokenType op, Expr *right)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    expr->type = EXPR_BINARY;
    expr->as.binary.left = left;
    expr->as.binary.right = right;
    expr->as.binary.operator = op;
    expr->expr_type = left->expr_type;  /* Simplified */
    return expr;
}

/* Helper to create a unary expression */
static Expr *create_unary_expr(Arena *arena, SnTokenType op, Expr *operand)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    expr->type = EXPR_UNARY;
    expr->as.unary.operator = op;
    expr->as.unary.operand = operand;
    expr->expr_type = operand->expr_type;
    return expr;
}

/* Helper to create a return statement */
static Stmt *create_return_stmt(Arena *arena, Expr *value)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    stmt->type = STMT_RETURN;
    Token tok;
    setup_basic_token(&tok, TOKEN_RETURN, "return");
    stmt->as.return_stmt.keyword = tok;
    stmt->as.return_stmt.value = value;
    return stmt;
}

/* Helper to create an expression statement */
static Stmt *create_expr_stmt(Arena *arena, Expr *expr)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    stmt->type = STMT_EXPR;
    stmt->as.expression.expression = expr;
    return stmt;
}

/* Helper to create a variable declaration */
static Stmt *create_var_decl(Arena *arena, const char *name, Expr *init)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    stmt->type = STMT_VAR_DECL;
    setup_basic_token(&stmt->as.var_decl.name, TOKEN_IDENTIFIER, name);
    stmt->as.var_decl.type = ast_create_primitive_type(arena, TYPE_INT);
    stmt->as.var_decl.initializer = init;
    stmt->as.var_decl.mem_qualifier = MEM_DEFAULT;
    return stmt;
}

/* Helper to create a call expression */
static Expr *create_call_expr(Arena *arena, const char *func_name, Expr **args, int arg_count)
{
    Expr *callee = arena_alloc(arena, sizeof(Expr));
    callee->type = EXPR_VARIABLE;
    setup_basic_token(&callee->as.variable.name, TOKEN_IDENTIFIER, func_name);
    callee->expr_type = ast_create_primitive_type(arena, TYPE_INT);

    Expr *call = arena_alloc(arena, sizeof(Expr));
    call->type = EXPR_CALL;
    call->as.call.callee = callee;
    call->as.call.arguments = args;
    call->as.call.arg_count = arg_count;
    call->as.call.is_tail_call = false;
    call->expr_type = ast_create_primitive_type(arena, TYPE_INT);
    return call;
}

/* Helper to create a string literal expression */
static Expr *create_string_literal(Arena *arena, const char *value)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    expr->type = EXPR_LITERAL;
    expr->as.literal.type = ast_create_primitive_type(arena, TYPE_STRING);
    expr->as.literal.value.string_value = arena_strdup(arena, value);
    expr->expr_type = expr->as.literal.type;
    return expr;
}

