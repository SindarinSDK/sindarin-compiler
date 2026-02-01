// optimizer_util_dead.c
// Dead code removal and simplification utilities

/* ============================================================================
 * Dead Code Removal Helpers
 * ============================================================================
 */

int remove_unused_variables(Optimizer *opt, Stmt **stmts, int *count)
{
    if (stmts == NULL || *count <= 0) return 0;

    /* First, collect all variable uses in the entire block */
    Token *used_vars = NULL;
    int used_count = 0;
    int used_capacity = 0;

    for (int i = 0; i < *count; i++)
    {
        collect_used_variables_stmt(stmts[i], &used_vars, &used_count, &used_capacity, opt->arena);
    }

    /* Now filter out unused variable declarations */
    int removed = 0;
    int new_count = 0;

    for (int i = 0; i < *count; i++)
    {
        Stmt *stmt = stmts[i];

        if (stmt->type == STMT_VAR_DECL)
        {
            /* Check if this variable is used */
            if (!is_variable_used(used_vars, used_count, stmt->as.var_decl.name))
            {
                /* Variable is not used - but we can only remove it if:
                   1. It has no initializer, OR
                   2. The initializer has no side effects */
                Expr *init = stmt->as.var_decl.initializer;
                bool has_side_effects = false;

                if (init != NULL)
                {
                    /* Conservative: assume function calls and thread operations have side effects */
                    switch (init->type)
                    {
                    case EXPR_CALL:
                    case EXPR_INCREMENT:
                    case EXPR_DECREMENT:
                    case EXPR_ASSIGN:
                    case EXPR_INDEX_ASSIGN:
                    case EXPR_THREAD_SPAWN:
                    case EXPR_THREAD_SYNC:
                        has_side_effects = true;
                        break;
                    default:
                        has_side_effects = false;
                    }
                }

                if (!has_side_effects)
                {
                    removed++;
                    continue;  /* Skip this declaration */
                }
            }
        }

        /* Keep this statement */
        stmts[new_count] = stmt;
        new_count++;
    }

    *count = new_count;
    opt->variables_removed += removed;
    return removed;
}

Expr *simplify_noop_expr(Optimizer *opt, Expr *expr)
{
    if (expr == NULL) return NULL;

    /* First, recursively simplify sub-expressions */
    switch (expr->type)
    {
    case EXPR_BINARY:
        expr->as.binary.left = simplify_noop_expr(opt, expr->as.binary.left);
        expr->as.binary.right = simplify_noop_expr(opt, expr->as.binary.right);
        break;

    case EXPR_UNARY:
        expr->as.unary.operand = simplify_noop_expr(opt, expr->as.unary.operand);
        break;

    case EXPR_ASSIGN:
        expr->as.assign.value = simplify_noop_expr(opt, expr->as.assign.value);
        break;

    case EXPR_INDEX_ASSIGN:
        expr->as.index_assign.array = simplify_noop_expr(opt, expr->as.index_assign.array);
        expr->as.index_assign.index = simplify_noop_expr(opt, expr->as.index_assign.index);
        expr->as.index_assign.value = simplify_noop_expr(opt, expr->as.index_assign.value);
        break;

    case EXPR_CALL:
        expr->as.call.callee = simplify_noop_expr(opt, expr->as.call.callee);
        for (int i = 0; i < expr->as.call.arg_count; i++)
        {
            expr->as.call.arguments[i] = simplify_noop_expr(opt, expr->as.call.arguments[i]);
        }
        break;

    case EXPR_ARRAY:
        for (int i = 0; i < expr->as.array.element_count; i++)
        {
            expr->as.array.elements[i] = simplify_noop_expr(opt, expr->as.array.elements[i]);
        }
        break;

    case EXPR_ARRAY_ACCESS:
        expr->as.array_access.array = simplify_noop_expr(opt, expr->as.array_access.array);
        expr->as.array_access.index = simplify_noop_expr(opt, expr->as.array_access.index);
        break;

    case EXPR_ARRAY_SLICE:
        expr->as.array_slice.array = simplify_noop_expr(opt, expr->as.array_slice.array);
        if (expr->as.array_slice.start)
            expr->as.array_slice.start = simplify_noop_expr(opt, expr->as.array_slice.start);
        if (expr->as.array_slice.end)
            expr->as.array_slice.end = simplify_noop_expr(opt, expr->as.array_slice.end);
        if (expr->as.array_slice.step)
            expr->as.array_slice.step = simplify_noop_expr(opt, expr->as.array_slice.step);
        break;

    case EXPR_RANGE:
        expr->as.range.start = simplify_noop_expr(opt, expr->as.range.start);
        expr->as.range.end = simplify_noop_expr(opt, expr->as.range.end);
        break;

    case EXPR_SPREAD:
        expr->as.spread.array = simplify_noop_expr(opt, expr->as.spread.array);
        break;

    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        expr->as.operand = simplify_noop_expr(opt, expr->as.operand);
        break;

    case EXPR_INTERPOLATED:
        for (int i = 0; i < expr->as.interpol.part_count; i++)
        {
            expr->as.interpol.parts[i] = simplify_noop_expr(opt, expr->as.interpol.parts[i]);
        }
        break;

    case EXPR_MEMBER:
        expr->as.member.object = simplify_noop_expr(opt, expr->as.member.object);
        break;

    case EXPR_SIZED_ARRAY_ALLOC:
        expr->as.sized_array_alloc.size_expr = simplify_noop_expr(opt, expr->as.sized_array_alloc.size_expr);
        if (expr->as.sized_array_alloc.default_value)
        {
            expr->as.sized_array_alloc.default_value = simplify_noop_expr(opt, expr->as.sized_array_alloc.default_value);
        }
        break;

    case EXPR_STRUCT_LITERAL:
        for (int i = 0; i < expr->as.struct_literal.field_count; i++)
        {
            expr->as.struct_literal.fields[i].value = simplify_noop_expr(opt, expr->as.struct_literal.fields[i].value);
        }
        break;

    case EXPR_MEMBER_ACCESS:
        expr->as.member_access.object = simplify_noop_expr(opt, expr->as.member_access.object);
        break;

    case EXPR_MEMBER_ASSIGN:
        expr->as.member_assign.object = simplify_noop_expr(opt, expr->as.member_assign.object);
        expr->as.member_assign.value = simplify_noop_expr(opt, expr->as.member_assign.value);
        break;

    case EXPR_LAMBDA:
        if (expr->as.lambda.body)
        {
            expr->as.lambda.body = simplify_noop_expr(opt, expr->as.lambda.body);
        }
        for (int i = 0; i < expr->as.lambda.body_stmt_count; i++)
        {
            simplify_noop_stmt(opt, expr->as.lambda.body_stmts[i]);
        }
        break;

    case EXPR_STATIC_CALL:
        for (int i = 0; i < expr->as.static_call.arg_count; i++)
        {
            expr->as.static_call.arguments[i] = simplify_noop_expr(opt, expr->as.static_call.arguments[i]);
        }
        break;

    case EXPR_THREAD_SPAWN:
        expr->as.thread_spawn.call = simplify_noop_expr(opt, expr->as.thread_spawn.call);
        break;

    case EXPR_THREAD_SYNC:
        expr->as.thread_sync.handle = simplify_noop_expr(opt, expr->as.thread_sync.handle);
        break;

    case EXPR_SYNC_LIST:
        for (int i = 0; i < expr->as.sync_list.element_count; i++)
        {
            expr->as.sync_list.elements[i] = simplify_noop_expr(opt, expr->as.sync_list.elements[i]);
        }
        break;

    case EXPR_AS_VAL:
        expr->as.as_val.operand = simplify_noop_expr(opt, expr->as.as_val.operand);
        break;

    case EXPR_AS_REF:
        expr->as.as_ref.operand = simplify_noop_expr(opt, expr->as.as_ref.operand);
        break;

    case EXPR_TYPEOF:
        if (expr->as.typeof_expr.operand)
        {
            expr->as.typeof_expr.operand = simplify_noop_expr(opt, expr->as.typeof_expr.operand);
        }
        break;

    case EXPR_IS:
        expr->as.is_expr.operand = simplify_noop_expr(opt, expr->as.is_expr.operand);
        break;

    case EXPR_AS_TYPE:
        expr->as.as_type.operand = simplify_noop_expr(opt, expr->as.as_type.operand);
        break;

    case EXPR_SIZEOF:
        if (expr->as.sizeof_expr.expr_operand)
        {
            expr->as.sizeof_expr.expr_operand = simplify_noop_expr(opt, expr->as.sizeof_expr.expr_operand);
        }
        break;

    case EXPR_COMPOUND_ASSIGN:
        expr->as.compound_assign.target = simplify_noop_expr(opt, expr->as.compound_assign.target);
        expr->as.compound_assign.value = simplify_noop_expr(opt, expr->as.compound_assign.value);
        break;

    case EXPR_METHOD_CALL:
        if (expr->as.method_call.object)
        {
            expr->as.method_call.object = simplify_noop_expr(opt, expr->as.method_call.object);
        }
        for (int i = 0; i < expr->as.method_call.arg_count; i++)
        {
            expr->as.method_call.args[i] = simplify_noop_expr(opt, expr->as.method_call.args[i]);
        }
        break;

    case EXPR_MATCH:
        expr->as.match_expr.subject = simplify_noop_expr(opt, expr->as.match_expr.subject);
        for (int i = 0; i < expr->as.match_expr.arm_count; i++)
        {
            MatchArm *arm = &expr->as.match_expr.arms[i];
            if (!arm->is_else)
            {
                for (int j = 0; j < arm->pattern_count; j++)
                {
                    arm->patterns[j] = simplify_noop_expr(opt, arm->patterns[j]);
                }
            }
            if (arm->body != NULL)
            {
                simplify_noop_stmt(opt, arm->body);
            }
        }
        break;

    default:
        break;
    }

    /* Now check if this expression itself is a no-op */
    Expr *simplified;
    if (expr_is_noop(expr, &simplified))
    {
        opt->noops_removed++;
        return simplified;
    }

    return expr;
}

void simplify_noop_stmt(Optimizer *opt, Stmt *stmt)
{
    if (stmt == NULL) return;

    switch (stmt->type)
    {
    case STMT_EXPR:
        stmt->as.expression.expression = simplify_noop_expr(opt, stmt->as.expression.expression);
        break;

    case STMT_VAR_DECL:
        if (stmt->as.var_decl.initializer)
        {
            stmt->as.var_decl.initializer = simplify_noop_expr(opt, stmt->as.var_decl.initializer);
        }
        break;

    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
        {
            stmt->as.return_stmt.value = simplify_noop_expr(opt, stmt->as.return_stmt.value);
        }
        break;

    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            simplify_noop_stmt(opt, stmt->as.block.statements[i]);
        }
        break;

    case STMT_IF:
        stmt->as.if_stmt.condition = simplify_noop_expr(opt, stmt->as.if_stmt.condition);
        simplify_noop_stmt(opt, stmt->as.if_stmt.then_branch);
        if (stmt->as.if_stmt.else_branch)
        {
            simplify_noop_stmt(opt, stmt->as.if_stmt.else_branch);
        }
        break;

    case STMT_WHILE:
        stmt->as.while_stmt.condition = simplify_noop_expr(opt, stmt->as.while_stmt.condition);
        simplify_noop_stmt(opt, stmt->as.while_stmt.body);
        break;

    case STMT_FOR:
        if (stmt->as.for_stmt.initializer)
        {
            simplify_noop_stmt(opt, stmt->as.for_stmt.initializer);
        }
        if (stmt->as.for_stmt.condition)
        {
            stmt->as.for_stmt.condition = simplify_noop_expr(opt, stmt->as.for_stmt.condition);
        }
        if (stmt->as.for_stmt.increment)
        {
            stmt->as.for_stmt.increment = simplify_noop_expr(opt, stmt->as.for_stmt.increment);
        }
        simplify_noop_stmt(opt, stmt->as.for_stmt.body);
        break;

    case STMT_FOR_EACH:
        stmt->as.for_each_stmt.iterable = simplify_noop_expr(opt, stmt->as.for_each_stmt.iterable);
        simplify_noop_stmt(opt, stmt->as.for_each_stmt.body);
        break;

    case STMT_LOCK:
        stmt->as.lock_stmt.lock_expr = simplify_noop_expr(opt, stmt->as.lock_stmt.lock_expr);
        simplify_noop_stmt(opt, stmt->as.lock_stmt.body);
        break;

    default:
        break;
    }
}
