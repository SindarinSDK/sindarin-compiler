/* ==============================================================================
 * code_gen_expr_lambda_capture.c - Captured Variable Collection
 * ==============================================================================
 * Functions for collecting and managing captured variables in lambdas.
 * ============================================================================== */

/* Initialize captured vars structure */
void captured_vars_init(CapturedVars *cv)
{
    cv->names = NULL;
    cv->types = NULL;
    cv->count = 0;
    cv->capacity = 0;
}

/* Add a captured variable (de-duplicated) */
void captured_vars_add(CapturedVars *cv, Arena *arena, const char *name, Type *type)
{
    /* Check if already captured */
    for (int i = 0; i < cv->count; i++)
    {
        if (strcmp(cv->names[i], name) == 0)
            return;
    }
    /* Grow arrays if needed */
    if (cv->count >= cv->capacity)
    {
        int new_cap = cv->capacity == 0 ? 4 : cv->capacity * 2;
        char **new_names = arena_alloc(arena, new_cap * sizeof(char *));
        Type **new_types = arena_alloc(arena, new_cap * sizeof(Type *));
        for (int i = 0; i < cv->count; i++)
        {
            new_names[i] = cv->names[i];
            new_types[i] = cv->types[i];
        }
        cv->names = new_names;
        cv->types = new_types;
        cv->capacity = new_cap;
    }
    cv->names[cv->count] = arena_strdup(arena, name);
    cv->types[cv->count] = type;
    cv->count++;
}

/* Forward declaration for mutual recursion */
static void collect_captured_vars_internal(Expr *expr, LambdaExpr *lambda, SymbolTable *table,
                                           CapturedVars *cv, LocalVars *lv, EnclosingLambdaContext *enclosing, Arena *arena);

/* Recursively collect captured variables from a statement */
void collect_captured_vars_from_stmt(Stmt *stmt, LambdaExpr *lambda, SymbolTable *table,
                                     CapturedVars *cv, LocalVars *lv, EnclosingLambdaContext *enclosing, Arena *arena)
{
    if (stmt == NULL) return;

    switch (stmt->type)
    {
    case STMT_EXPR:
        collect_captured_vars_internal(stmt->as.expression.expression, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_VAR_DECL:
        if (stmt->as.var_decl.initializer)
            collect_captured_vars_internal(stmt->as.var_decl.initializer, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
            collect_captured_vars_internal(stmt->as.return_stmt.value, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
            collect_captured_vars_from_stmt(stmt->as.block.statements[i], lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_IF:
        collect_captured_vars_internal(stmt->as.if_stmt.condition, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_from_stmt(stmt->as.if_stmt.then_branch, lambda, table, cv, lv, enclosing, arena);
        if (stmt->as.if_stmt.else_branch)
            collect_captured_vars_from_stmt(stmt->as.if_stmt.else_branch, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_WHILE:
        collect_captured_vars_internal(stmt->as.while_stmt.condition, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_from_stmt(stmt->as.while_stmt.body, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_FOR:
        if (stmt->as.for_stmt.initializer)
            collect_captured_vars_from_stmt(stmt->as.for_stmt.initializer, lambda, table, cv, lv, enclosing, arena);
        if (stmt->as.for_stmt.condition)
            collect_captured_vars_internal(stmt->as.for_stmt.condition, lambda, table, cv, lv, enclosing, arena);
        if (stmt->as.for_stmt.increment)
            collect_captured_vars_internal(stmt->as.for_stmt.increment, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_from_stmt(stmt->as.for_stmt.body, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_FOR_EACH:
        collect_captured_vars_internal(stmt->as.for_each_stmt.iterable, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_from_stmt(stmt->as.for_each_stmt.body, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_LOCK:
        collect_captured_vars_internal(stmt->as.lock_stmt.lock_expr, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_from_stmt(stmt->as.lock_stmt.body, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_FUNCTION:
        /* Don't recurse into nested functions - they have their own scope */
        break;
    case STMT_BREAK:
    case STMT_CONTINUE:
    case STMT_IMPORT:
    default:
        break;
    }
}

/* Internal implementation for mutual recursion */
static void collect_captured_vars_internal(Expr *expr, LambdaExpr *lambda, SymbolTable *table,
                                           CapturedVars *cv, LocalVars *lv, EnclosingLambdaContext *enclosing, Arena *arena)
{
    if (expr == NULL) return;

    switch (expr->type)
    {
    case EXPR_VARIABLE:
    {
        char name[256];
        int len = expr->as.variable.name.length < 255 ? expr->as.variable.name.length : 255;
        strncpy(name, expr->as.variable.name.start, len);
        name[len] = '\0';

        /* Skip if it's a lambda parameter */
        if (is_lambda_param(lambda, name))
            return;

        /* Skip if it's a local variable declared in the lambda body */
        if (lv != NULL && is_local_var(lv, name))
            return;

        /* Skip builtins */
        if (strcmp(name, "print") == 0 || strcmp(name, "len") == 0)
            return;

        /* Look up in symbol table to see if it's an outer variable */
        Symbol *sym = symbol_table_lookup_symbol(table, expr->as.variable.name);
        if (sym != NULL)
        {
            /* It's a captured variable from outer scope */
            captured_vars_add(cv, arena, name, sym->type);
        }
        else
        {
            /* Check if it's a parameter from an enclosing lambda */
            Type *enclosing_type = find_enclosing_lambda_param(enclosing, name);
            if (enclosing_type != NULL)
            {
                captured_vars_add(cv, arena, name, enclosing_type);
            }
        }
        break;
    }
    case EXPR_BINARY:
        collect_captured_vars_internal(expr->as.binary.left, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_internal(expr->as.binary.right, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_UNARY:
        collect_captured_vars_internal(expr->as.unary.operand, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_ASSIGN:
        collect_captured_vars_internal(expr->as.assign.value, lambda, table, cv, lv, enclosing, arena);
        /* Also capture the assignment target variable itself */
        {
            char name[256];
            int len = expr->as.assign.name.length < 255 ? expr->as.assign.name.length : 255;
            strncpy(name, expr->as.assign.name.start, len);
            name[len] = '\0';
            if (!is_lambda_param(lambda, name) && !(lv != NULL && is_local_var(lv, name)))
            {
                Symbol *sym = symbol_table_lookup_symbol(table, expr->as.assign.name);
                if (sym != NULL)
                {
                    captured_vars_add(cv, arena, name, sym->type);
                }
            }
        }
        break;
    case EXPR_COMPOUND_ASSIGN:
        collect_captured_vars_internal(expr->as.compound_assign.target, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_internal(expr->as.compound_assign.value, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_INDEX_ASSIGN:
        collect_captured_vars_internal(expr->as.index_assign.array, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_internal(expr->as.index_assign.index, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_internal(expr->as.index_assign.value, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_CALL:
        collect_captured_vars_internal(expr->as.call.callee, lambda, table, cv, lv, enclosing, arena);
        for (int i = 0; i < expr->as.call.arg_count; i++)
            collect_captured_vars_internal(expr->as.call.arguments[i], lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_ARRAY:
        for (int i = 0; i < expr->as.array.element_count; i++)
            collect_captured_vars_internal(expr->as.array.elements[i], lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_ARRAY_ACCESS:
        collect_captured_vars_internal(expr->as.array_access.array, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_internal(expr->as.array_access.index, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        collect_captured_vars_internal(expr->as.operand, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_INTERPOLATED:
        for (int i = 0; i < expr->as.interpol.part_count; i++)
            collect_captured_vars_internal(expr->as.interpol.parts[i], lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_MEMBER:
        collect_captured_vars_internal(expr->as.member.object, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_ARRAY_SLICE:
        collect_captured_vars_internal(expr->as.array_slice.array, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_internal(expr->as.array_slice.start, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_internal(expr->as.array_slice.end, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_internal(expr->as.array_slice.step, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_RANGE:
        collect_captured_vars_internal(expr->as.range.start, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_internal(expr->as.range.end, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_SPREAD:
        collect_captured_vars_internal(expr->as.spread.array, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_LAMBDA:
        /* Recurse into nested lambdas to collect transitive captures */
        /* Variables captured by nested lambdas that are from outer scopes
           need to be captured by this lambda too */
        {
            LambdaExpr *nested_lambda = &expr->as.lambda;
            if (nested_lambda->has_stmt_body)
            {
                for (int i = 0; i < nested_lambda->body_stmt_count; i++)
                {
                    collect_captured_vars_from_stmt(nested_lambda->body_stmts[i], lambda, table, cv, lv, enclosing, arena);
                }
            }
            else if (nested_lambda->body)
            {
                collect_captured_vars_internal(nested_lambda->body, lambda, table, cv, lv, enclosing, arena);
            }
        }
        break;
    case EXPR_STATIC_CALL:
        for (int i = 0; i < expr->as.static_call.arg_count; i++)
        {
            collect_captured_vars_internal(expr->as.static_call.arguments[i], lambda, table, cv, lv, enclosing, arena);
        }
        break;
    case EXPR_MATCH:
        collect_captured_vars_internal(expr->as.match_expr.subject, lambda, table, cv, lv, enclosing, arena);
        for (int i = 0; i < expr->as.match_expr.arm_count; i++)
        {
            MatchArm *arm = &expr->as.match_expr.arms[i];
            if (!arm->is_else)
            {
                for (int j = 0; j < arm->pattern_count; j++)
                    collect_captured_vars_internal(arm->patterns[j], lambda, table, cv, lv, enclosing, arena);
            }
            if (arm->body != NULL)
                collect_captured_vars_from_stmt(arm->body, lambda, table, cv, lv, enclosing, arena);
        }
        break;
    case EXPR_LITERAL:
    default:
        break;
    }
}

/* Public wrapper for collect_captured_vars */
void collect_captured_vars(Expr *expr, LambdaExpr *lambda, SymbolTable *table,
                           CapturedVars *cv, LocalVars *lv, EnclosingLambdaContext *enclosing, Arena *arena)
{
    collect_captured_vars_internal(expr, lambda, table, cv, lv, enclosing, arena);
}
