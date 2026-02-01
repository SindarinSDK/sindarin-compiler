// optimizer_util_vars.c
// Variable usage tracking utilities

/* ============================================================================
 * Variable Usage Tracking
 * ============================================================================
 */

void add_used_variable(Token **used_vars, int *used_count, int *used_capacity,
                       Arena *arena, Token name)
{
    /* Check if already in list */
    for (int i = 0; i < *used_count; i++)
    {
        if ((*used_vars)[i].length == name.length &&
            strncmp((*used_vars)[i].start, name.start, name.length) == 0)
        {
            return;  /* Already tracked */
        }
    }

    /* Grow array if needed */
    if (*used_count >= *used_capacity)
    {
        int new_cap = (*used_capacity == 0) ? 16 : (*used_capacity * 2);
        Token *new_vars = arena_alloc(arena, new_cap * sizeof(Token));
        for (int i = 0; i < *used_count; i++)
        {
            new_vars[i] = (*used_vars)[i];
        }
        *used_vars = new_vars;
        *used_capacity = new_cap;
    }

    (*used_vars)[*used_count] = name;
    (*used_count)++;
}

void collect_used_variables(Expr *expr, Token **used_vars, int *used_count,
                            int *used_capacity, Arena *arena)
{
    if (expr == NULL) return;

    switch (expr->type)
    {
    case EXPR_VARIABLE:
        add_used_variable(used_vars, used_count, used_capacity, arena, expr->as.variable.name);
        break;

    case EXPR_BINARY:
        collect_used_variables(expr->as.binary.left, used_vars, used_count, used_capacity, arena);
        collect_used_variables(expr->as.binary.right, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_UNARY:
        collect_used_variables(expr->as.unary.operand, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_ASSIGN:
        /* The variable being assigned TO is not a "use" (it's a def),
           but the value being assigned IS a use */
        collect_used_variables(expr->as.assign.value, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_INDEX_ASSIGN:
        collect_used_variables(expr->as.index_assign.array, used_vars, used_count, used_capacity, arena);
        collect_used_variables(expr->as.index_assign.index, used_vars, used_count, used_capacity, arena);
        collect_used_variables(expr->as.index_assign.value, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_CALL:
        collect_used_variables(expr->as.call.callee, used_vars, used_count, used_capacity, arena);
        for (int i = 0; i < expr->as.call.arg_count; i++)
        {
            collect_used_variables(expr->as.call.arguments[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_ARRAY:
        for (int i = 0; i < expr->as.array.element_count; i++)
        {
            collect_used_variables(expr->as.array.elements[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_ARRAY_ACCESS:
        collect_used_variables(expr->as.array_access.array, used_vars, used_count, used_capacity, arena);
        collect_used_variables(expr->as.array_access.index, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_ARRAY_SLICE:
        collect_used_variables(expr->as.array_slice.array, used_vars, used_count, used_capacity, arena);
        if (expr->as.array_slice.start)
            collect_used_variables(expr->as.array_slice.start, used_vars, used_count, used_capacity, arena);
        if (expr->as.array_slice.end)
            collect_used_variables(expr->as.array_slice.end, used_vars, used_count, used_capacity, arena);
        if (expr->as.array_slice.step)
            collect_used_variables(expr->as.array_slice.step, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_RANGE:
        collect_used_variables(expr->as.range.start, used_vars, used_count, used_capacity, arena);
        collect_used_variables(expr->as.range.end, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_SPREAD:
        collect_used_variables(expr->as.spread.array, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        collect_used_variables(expr->as.operand, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_INTERPOLATED:
        for (int i = 0; i < expr->as.interpol.part_count; i++)
        {
            collect_used_variables(expr->as.interpol.parts[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_MEMBER:
        collect_used_variables(expr->as.member.object, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_LAMBDA:
        /* Lambda bodies should track their own variables, but captured
           variables from outer scope count as uses */
        if (expr->as.lambda.body)
        {
            collect_used_variables(expr->as.lambda.body, used_vars, used_count, used_capacity, arena);
        }
        for (int i = 0; i < expr->as.lambda.body_stmt_count; i++)
        {
            collect_used_variables_stmt(expr->as.lambda.body_stmts[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_STATIC_CALL:
        for (int i = 0; i < expr->as.static_call.arg_count; i++)
        {
            collect_used_variables(expr->as.static_call.arguments[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_SIZED_ARRAY_ALLOC:
        /* Sized array allocations use both the size expression and default value */
        collect_used_variables(expr->as.sized_array_alloc.size_expr, used_vars, used_count, used_capacity, arena);
        if (expr->as.sized_array_alloc.default_value)
        {
            collect_used_variables(expr->as.sized_array_alloc.default_value, used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_THREAD_SPAWN:
        /* Thread spawn wraps a function call - collect variables from the call */
        collect_used_variables(expr->as.thread_spawn.call, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_THREAD_SYNC:
        /* Thread sync uses the handle expression */
        collect_used_variables(expr->as.thread_sync.handle, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_TYPEOF:
        /* typeof uses its operand expression */
        if (expr->as.typeof_expr.operand)
        {
            collect_used_variables(expr->as.typeof_expr.operand, used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_IS:
        /* is uses its operand expression */
        collect_used_variables(expr->as.is_expr.operand, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_AS_TYPE:
        /* as uses its operand expression */
        collect_used_variables(expr->as.as_type.operand, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_AS_VAL:
        /* as val uses its operand expression */
        collect_used_variables(expr->as.as_val.operand, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_AS_REF:
        /* as ref uses its operand expression */
        collect_used_variables(expr->as.as_ref.operand, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_STRUCT_LITERAL:
        /* Struct literal field value expressions use variables */
        for (int i = 0; i < expr->as.struct_literal.field_count; i++)
        {
            collect_used_variables(expr->as.struct_literal.fields[i].value, used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_MEMBER_ACCESS:
        /* Member access uses the object expression */
        collect_used_variables(expr->as.member_access.object, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_MEMBER_ASSIGN:
        /* Member assignment uses both the object and value expressions */
        collect_used_variables(expr->as.member_assign.object, used_vars, used_count, used_capacity, arena);
        collect_used_variables(expr->as.member_assign.value, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_MATCH:
        /* Match uses subject and pattern expressions, and arm bodies */
        collect_used_variables(expr->as.match_expr.subject, used_vars, used_count, used_capacity, arena);
        for (int i = 0; i < expr->as.match_expr.arm_count; i++)
        {
            MatchArm *arm = &expr->as.match_expr.arms[i];
            if (!arm->is_else)
            {
                for (int j = 0; j < arm->pattern_count; j++)
                    collect_used_variables(arm->patterns[j], used_vars, used_count, used_capacity, arena);
            }
            if (arm->body != NULL)
                collect_used_variables_stmt(arm->body, used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_SYNC_LIST:
        /* Sync list contains expressions to synchronize */
        for (int i = 0; i < expr->as.sync_list.element_count; i++)
        {
            collect_used_variables(expr->as.sync_list.elements[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_SIZEOF:
        /* sizeof may have an expression operand */
        if (expr->as.sizeof_expr.expr_operand)
        {
            collect_used_variables(expr->as.sizeof_expr.expr_operand, used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_COMPOUND_ASSIGN:
        /* Compound assignment uses both target and value */
        collect_used_variables(expr->as.compound_assign.target, used_vars, used_count, used_capacity, arena);
        collect_used_variables(expr->as.compound_assign.value, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_METHOD_CALL:
        /* Method call uses object (if not static) and arguments */
        if (expr->as.method_call.object)
        {
            collect_used_variables(expr->as.method_call.object, used_vars, used_count, used_capacity, arena);
        }
        for (int i = 0; i < expr->as.method_call.arg_count; i++)
        {
            collect_used_variables(expr->as.method_call.args[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_LITERAL:
    default:
        break;
    }
}

void collect_used_variables_stmt(Stmt *stmt, Token **used_vars, int *used_count,
                                 int *used_capacity, Arena *arena)
{
    if (stmt == NULL) return;

    switch (stmt->type)
    {
    case STMT_EXPR:
        collect_used_variables(stmt->as.expression.expression, used_vars, used_count, used_capacity, arena);
        break;

    case STMT_VAR_DECL:
        /* The variable being declared is not a use, but the initializer is */
        if (stmt->as.var_decl.initializer)
        {
            collect_used_variables(stmt->as.var_decl.initializer, used_vars, used_count, used_capacity, arena);
        }
        break;

    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
        {
            collect_used_variables(stmt->as.return_stmt.value, used_vars, used_count, used_capacity, arena);
        }
        break;

    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            collect_used_variables_stmt(stmt->as.block.statements[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case STMT_IF:
        collect_used_variables(stmt->as.if_stmt.condition, used_vars, used_count, used_capacity, arena);
        collect_used_variables_stmt(stmt->as.if_stmt.then_branch, used_vars, used_count, used_capacity, arena);
        if (stmt->as.if_stmt.else_branch)
        {
            collect_used_variables_stmt(stmt->as.if_stmt.else_branch, used_vars, used_count, used_capacity, arena);
        }
        break;

    case STMT_WHILE:
        collect_used_variables(stmt->as.while_stmt.condition, used_vars, used_count, used_capacity, arena);
        collect_used_variables_stmt(stmt->as.while_stmt.body, used_vars, used_count, used_capacity, arena);
        break;

    case STMT_FOR:
        if (stmt->as.for_stmt.initializer)
        {
            collect_used_variables_stmt(stmt->as.for_stmt.initializer, used_vars, used_count, used_capacity, arena);
        }
        if (stmt->as.for_stmt.condition)
        {
            collect_used_variables(stmt->as.for_stmt.condition, used_vars, used_count, used_capacity, arena);
        }
        if (stmt->as.for_stmt.increment)
        {
            collect_used_variables(stmt->as.for_stmt.increment, used_vars, used_count, used_capacity, arena);
        }
        collect_used_variables_stmt(stmt->as.for_stmt.body, used_vars, used_count, used_capacity, arena);
        break;

    case STMT_FOR_EACH:
        collect_used_variables(stmt->as.for_each_stmt.iterable, used_vars, used_count, used_capacity, arena);
        collect_used_variables_stmt(stmt->as.for_each_stmt.body, used_vars, used_count, used_capacity, arena);
        break;

    case STMT_FUNCTION:
        /* Don't descend into nested function definitions for variable tracking */
        break;

    case STMT_LOCK:
        /* Lock statement uses the lock expression and body */
        collect_used_variables(stmt->as.lock_stmt.lock_expr, used_vars, used_count, used_capacity, arena);
        collect_used_variables_stmt(stmt->as.lock_stmt.body, used_vars, used_count, used_capacity, arena);
        break;

    case STMT_BREAK:
    case STMT_CONTINUE:
    case STMT_IMPORT:
    case STMT_PRAGMA:
    case STMT_TYPE_DECL:
    case STMT_STRUCT_DECL:
    default:
        break;
    }
}

bool is_variable_used(Token *used_vars, int used_count, Token name)
{
    for (int i = 0; i < used_count; i++)
    {
        if (used_vars[i].length == name.length &&
            strncmp(used_vars[i].start, name.start, name.length) == 0)
        {
            return true;
        }
    }
    return false;
}
