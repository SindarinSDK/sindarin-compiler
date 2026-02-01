/* ==============================================================================
 * code_gen_expr_lambda_local.c - Local Variable Handling
 * ==============================================================================
 * Functions for tracking local variables in lambda bodies.
 * ============================================================================== */

/* Initialize LocalVars structure */
void local_vars_init(LocalVars *lv)
{
    lv->names = NULL;
    lv->count = 0;
    lv->capacity = 0;
}

/* Add a local variable name (de-duplicated) */
void local_vars_add(LocalVars *lv, Arena *arena, const char *name)
{
    /* Check if already tracked */
    for (int i = 0; i < lv->count; i++)
    {
        if (strcmp(lv->names[i], name) == 0)
            return;
    }
    /* Grow array if needed */
    if (lv->count >= lv->capacity)
    {
        int new_cap = lv->capacity == 0 ? 8 : lv->capacity * 2;
        char **new_names = arena_alloc(arena, new_cap * sizeof(char *));
        for (int i = 0; i < lv->count; i++)
        {
            new_names[i] = lv->names[i];
        }
        lv->names = new_names;
        lv->capacity = new_cap;
    }
    lv->names[lv->count] = arena_strdup(arena, name);
    lv->count++;
}

/* Collect local variable declarations from a statement */
void collect_local_vars_from_stmt(Stmt *stmt, LocalVars *lv, Arena *arena)
{
    if (stmt == NULL) return;

    switch (stmt->type)
    {
    case STMT_VAR_DECL:
    {
        /* Add this variable to locals */
        char name[256];
        int len = stmt->as.var_decl.name.length < 255 ? stmt->as.var_decl.name.length : 255;
        strncpy(name, stmt->as.var_decl.name.start, len);
        name[len] = '\0';
        local_vars_add(lv, arena, name);
        break;
    }
    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
            collect_local_vars_from_stmt(stmt->as.block.statements[i], lv, arena);
        break;
    case STMT_IF:
        collect_local_vars_from_stmt(stmt->as.if_stmt.then_branch, lv, arena);
        if (stmt->as.if_stmt.else_branch)
            collect_local_vars_from_stmt(stmt->as.if_stmt.else_branch, lv, arena);
        break;
    case STMT_WHILE:
        collect_local_vars_from_stmt(stmt->as.while_stmt.body, lv, arena);
        break;
    case STMT_FOR:
        if (stmt->as.for_stmt.initializer)
            collect_local_vars_from_stmt(stmt->as.for_stmt.initializer, lv, arena);
        collect_local_vars_from_stmt(stmt->as.for_stmt.body, lv, arena);
        break;
    case STMT_FOR_EACH:
    {
        /* The loop variable is a local */
        char name[256];
        int len = stmt->as.for_each_stmt.var_name.length < 255 ? stmt->as.for_each_stmt.var_name.length : 255;
        strncpy(name, stmt->as.for_each_stmt.var_name.start, len);
        name[len] = '\0';
        local_vars_add(lv, arena, name);
        collect_local_vars_from_stmt(stmt->as.for_each_stmt.body, lv, arena);
        break;
    }
    case STMT_LOCK:
        collect_local_vars_from_stmt(stmt->as.lock_stmt.body, lv, arena);
        break;
    default:
        break;
    }
}

/* Check if a name is a parameter of any enclosing lambda, and get its type */
Type *find_enclosing_lambda_param(EnclosingLambdaContext *ctx, const char *name)
{
    if (ctx == NULL) return NULL;
    for (int i = 0; i < ctx->count; i++)
    {
        LambdaExpr *lambda = ctx->lambdas[i];
        for (int j = 0; j < lambda->param_count; j++)
        {
            char param_name[256];
            int len = lambda->params[j].name.length < 255 ? lambda->params[j].name.length : 255;
            strncpy(param_name, lambda->params[j].name.start, len);
            param_name[len] = '\0';
            if (strcmp(param_name, name) == 0)
            {
                return lambda->params[j].type;
            }
        }
    }
    return NULL;
}

/* Helper to check if a name is a lambda parameter */
bool is_lambda_param(LambdaExpr *lambda, const char *name)
{
    for (int i = 0; i < lambda->param_count; i++)
    {
        char param_name[256];
        int len = lambda->params[i].name.length < 255 ? lambda->params[i].name.length : 255;
        strncpy(param_name, lambda->params[i].name.start, len);
        param_name[len] = '\0';
        if (strcmp(param_name, name) == 0)
            return true;
    }
    return false;
}

/* Check if a name is a local variable in the current lambda scope */
bool is_local_var(LocalVars *lv, const char *name)
{
    for (int i = 0; i < lv->count; i++)
    {
        if (strcmp(lv->names[i], name) == 0)
            return true;
    }
    return false;
}
