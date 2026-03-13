#include "code_gen/util/code_gen_util.h"
#include <string.h>

/* ============================================================================
 * Arena Requirement Analysis
 * ============================================================================
 * These functions analyze AST nodes to determine if they require arena
 * allocation. Functions that only use primitives don't need to create/destroy
 * arenas, which reduces overhead.
 */

/* Check if a type requires arena allocation */
static bool type_needs_arena(Type *type)
{
    if (type == NULL) return false;

    switch (type->kind)
    {
    case TYPE_STRING:
    case TYPE_ARRAY:
    case TYPE_FUNCTION:  /* Closures need arena */
        return true;
    case TYPE_INT:
    case TYPE_INT32:
    case TYPE_UINT:
    case TYPE_UINT32:
    case TYPE_LONG:
    case TYPE_DOUBLE:
    case TYPE_FLOAT:
    case TYPE_CHAR:
    case TYPE_BOOL:
    case TYPE_BYTE:
    case TYPE_VOID:
    case TYPE_NIL:
    case TYPE_POINTER:  /* Pointers are raw C pointers, no arena needed */
    default:
        return false;
    case TYPE_OPAQUE:
        /* Runtime objects (Process, etc.) that may be allocated via arena.
         * Functions using these types need an arena even though the type
         * itself isn't a heap type like string/array. */
        return true;
    }
}

bool expr_needs_arena(Expr *expr)
{
    if (expr == NULL) return false;

    switch (expr->type)
    {
    case EXPR_LITERAL:
        /* String literals don't need arena when just reading them,
           but they do when assigned to variables (handled in var_decl) */
        return false;

    case EXPR_VARIABLE:
        /* Variable references don't need arena, even function references.
           The function's closure was already allocated elsewhere.
           The type check is skipped here - a function reference doesn't
           mean we're creating a closure.

           Exception: the 'arena' built-in identifier requires arena context. */
        if (expr->as.variable.name.length == 5 &&
            strncmp(expr->as.variable.name.start, "arena", 5) == 0)
        {
            return true;
        }
        return false;

    case EXPR_BINARY:
        /* String concatenation needs arena */
        if (expr->as.binary.left->expr_type &&
            expr->as.binary.left->expr_type->kind == TYPE_STRING)
        {
            return true;
        }
        return expr_needs_arena(expr->as.binary.left) ||
               expr_needs_arena(expr->as.binary.right);

    case EXPR_UNARY:
        return expr_needs_arena(expr->as.unary.operand);

    case EXPR_ASSIGN:
        return expr_needs_arena(expr->as.assign.value);

    case EXPR_INDEX_ASSIGN:
        return expr_needs_arena(expr->as.index_assign.array) ||
               expr_needs_arena(expr->as.index_assign.index) ||
               expr_needs_arena(expr->as.index_assign.value);

    case EXPR_CALL:
        /* Function calls may return strings/arrays */
        if (type_needs_arena(expr->expr_type))
        {
            return true;
        }
        /* Check arguments */
        for (int i = 0; i < expr->as.call.arg_count; i++)
        {
            if (expr_needs_arena(expr->as.call.arguments[i]))
            {
                return true;
            }
        }
        /* Check callee - but skip if it's a simple function reference.
           Only complex callees (like method calls or computed functions)
           might need arena allocation. */
        if (expr->as.call.callee->type != EXPR_VARIABLE)
        {
            return expr_needs_arena(expr->as.call.callee);
        }
        return false;

    case EXPR_ARRAY:
        /* Array literals need arena */
        return true;

    case EXPR_ARRAY_ACCESS:
        return expr_needs_arena(expr->as.array_access.array) ||
               expr_needs_arena(expr->as.array_access.index);

    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        return expr_needs_arena(expr->as.operand);

    case EXPR_INTERPOLATED:
        /* String interpolation always needs arena */
        return true;

    case EXPR_MEMBER:
        return expr_needs_arena(expr->as.member.object);

    case EXPR_ARRAY_SLICE:
        /* Slices create new arrays */
        return true;

    case EXPR_RANGE:
        /* Ranges create arrays */
        return true;

    case EXPR_SPREAD:
        return true;

    case EXPR_LAMBDA:
        /* Lambdas create closures */
        return true;

    case EXPR_THREAD_SPAWN:
        /* Thread spawns need arena for allocating thread args and result */
        return true;

    case EXPR_THREAD_SYNC:
        /* Thread sync may promote results to caller's arena */
        return true;

    default:
        return false;
    }
}

bool stmt_needs_arena(Stmt *stmt)
{
    if (stmt == NULL) return false;

    switch (stmt->type)
    {
    case STMT_EXPR:
        return expr_needs_arena(stmt->as.expression.expression);

    case STMT_VAR_DECL:
        /* Variable declarations with string/array types need arena */
        if (type_needs_arena(stmt->as.var_decl.type))
        {
            return true;
        }
        /* Also check initializer */
        if (stmt->as.var_decl.initializer)
        {
            return expr_needs_arena(stmt->as.var_decl.initializer);
        }
        /* 'as ref' needs arena for heap allocation */
        if (stmt->as.var_decl.mem_qualifier == MEM_AS_REF)
        {
            return true;
        }
        return false;

    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
        {
            return expr_needs_arena(stmt->as.return_stmt.value);
        }
        return false;

    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            if (stmt_needs_arena(stmt->as.block.statements[i]))
            {
                return true;
            }
        }
        return false;

    case STMT_IF:
        if (expr_needs_arena(stmt->as.if_stmt.condition))
        {
            return true;
        }
        if (stmt_needs_arena(stmt->as.if_stmt.then_branch))
        {
            return true;
        }
        if (stmt->as.if_stmt.else_branch &&
            stmt_needs_arena(stmt->as.if_stmt.else_branch))
        {
            return true;
        }
        return false;

    case STMT_WHILE:
        if (expr_needs_arena(stmt->as.while_stmt.condition))
        {
            return true;
        }
        return stmt_needs_arena(stmt->as.while_stmt.body);

    case STMT_FOR:
        if (stmt->as.for_stmt.initializer &&
            stmt_needs_arena(stmt->as.for_stmt.initializer))
        {
            return true;
        }
        if (stmt->as.for_stmt.condition &&
            expr_needs_arena(stmt->as.for_stmt.condition))
        {
            return true;
        }
        if (stmt->as.for_stmt.increment &&
            expr_needs_arena(stmt->as.for_stmt.increment))
        {
            return true;
        }
        return stmt_needs_arena(stmt->as.for_stmt.body);

    case STMT_FOR_EACH:
        /* For-each iterates over arrays/strings */
        return true;

    case STMT_LOCK:
        /* Check lock expression and body */
        if (expr_needs_arena(stmt->as.lock_stmt.lock_expr))
        {
            return true;
        }
        return stmt_needs_arena(stmt->as.lock_stmt.body);

    case STMT_USING:
        /* Check initializer and body */
        if (expr_needs_arena(stmt->as.using_stmt.initializer))
        {
            return true;
        }
        return stmt_needs_arena(stmt->as.using_stmt.body);

    case STMT_FUNCTION:
        /* Nested functions don't affect parent's arena needs */
        return false;

    case STMT_BREAK:
    case STMT_CONTINUE:
    case STMT_IMPORT:
    default:
        return false;
    }
}

bool function_needs_arena(FunctionStmt *fn)
{
    if (fn == NULL) return false;

    /* Check return type */
    if (type_needs_arena(fn->return_type))
    {
        return true;
    }

    /* Check parameters for 'as val' with string/array types */
    for (int i = 0; i < fn->param_count; i++)
    {
        if (fn->params[i].mem_qualifier == MEM_AS_VAL)
        {
            if (type_needs_arena(fn->params[i].type))
            {
                return true;
            }
        }
    }

    /* Check function body */
    for (int i = 0; i < fn->body_count; i++)
    {
        if (stmt_needs_arena(fn->body[i]))
        {
            return true;
        }
    }

    return false;
}

/* ============================================================================
 * Arena Strategy Inference for Free Functions
 * ============================================================================
 * Determines whether a free function needs its own child arena (ARENA_CHILD)
 * or can alias the caller's arena (ARENA_NONE).
 *
 * ARENA_NONE is safe when the function body contains NO expressions that
 * directly allocate on __local_arena__. Function calls are excluded because
 * callees manage their own arenas and promote return values to __caller_arena__.
 *
 * Conservative: any unknown or ambiguous pattern returns ARENA_CHILD.
 */

static bool stmt_allocates(Stmt *stmt);

/* Check if an expression directly allocates on the arena.
 * This is distinct from expr_needs_arena() — it only checks for direct
 * allocation patterns, NOT function calls that return heap types. */
static bool expr_allocates(Expr *expr)
{
    if (expr == NULL) return false;

    switch (expr->type)
    {
    /* Direct allocation patterns */
    case EXPR_LITERAL:
        return expr->expr_type != NULL && expr->expr_type->kind == TYPE_STRING;

    case EXPR_INTERPOLATED:
        return true;

    case EXPR_BINARY:
        if (expr->expr_type != NULL && expr->expr_type->kind == TYPE_STRING)
            return true;
        return expr_allocates(expr->as.binary.left) ||
               expr_allocates(expr->as.binary.right);

    case EXPR_ARRAY:
    case EXPR_STRUCT_LITERAL:
    case EXPR_SIZED_ARRAY_ALLOC:
    case EXPR_LAMBDA:
    case EXPR_COPY_OF:
    case EXPR_THREAD_SPAWN:
    case EXPR_ARRAY_SLICE:
    case EXPR_RANGE:
    case EXPR_SPREAD:
        return true;

    case EXPR_VALUE_OF:
        if (expr->as.value_of.is_cstr_to_str)
            return true;
        return expr_allocates(expr->as.value_of.operand);

    /* Compound expressions — recurse into sub-expressions */
    case EXPR_UNARY:
        return expr_allocates(expr->as.unary.operand);

    case EXPR_ASSIGN:
        return expr_allocates(expr->as.assign.value);

    case EXPR_INDEX_ASSIGN:
        return expr_allocates(expr->as.index_assign.array) ||
               expr_allocates(expr->as.index_assign.index) ||
               expr_allocates(expr->as.index_assign.value);

    case EXPR_CALL:
        /* Function calls do NOT trigger ARENA_CHILD — callees manage their own
         * arenas and promote return values to __caller_arena__. But check args. */
        for (int i = 0; i < expr->as.call.arg_count; i++)
        {
            if (expr_allocates(expr->as.call.arguments[i]))
                return true;
        }
        if (expr->as.call.callee->type != EXPR_VARIABLE)
            return expr_allocates(expr->as.call.callee);
        return false;

    case EXPR_METHOD_CALL:
        if (expr_allocates(expr->as.method_call.object))
            return true;
        for (int i = 0; i < expr->as.method_call.arg_count; i++)
        {
            if (expr_allocates(expr->as.method_call.args[i]))
                return true;
        }
        return false;

    case EXPR_STATIC_CALL:
        for (int i = 0; i < expr->as.static_call.arg_count; i++)
        {
            if (expr_allocates(expr->as.static_call.arguments[i]))
                return true;
        }
        return false;

    case EXPR_ARRAY_ACCESS:
        return expr_allocates(expr->as.array_access.array) ||
               expr_allocates(expr->as.array_access.index);

    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        return expr_allocates(expr->as.operand);

    case EXPR_MEMBER:
        return expr_allocates(expr->as.member.object);

    case EXPR_COMPOUND_ASSIGN:
        return expr_allocates(expr->as.compound_assign.value);

    case EXPR_MEMBER_ASSIGN:
        return expr_allocates(expr->as.member_assign.value);

    case EXPR_MEMBER_ACCESS:
        return expr_allocates(expr->as.member_access.object);

    case EXPR_MATCH:
        if (expr_allocates(expr->as.match_expr.subject))
            return true;
        for (int i = 0; i < expr->as.match_expr.arm_count; i++)
        {
            for (int j = 0; j < expr->as.match_expr.arms[i].pattern_count; j++)
            {
                if (expr_allocates(expr->as.match_expr.arms[i].patterns[j]))
                    return true;
            }
            if (stmt_allocates(expr->as.match_expr.arms[i].body))
                return true;
        }
        return false;

    /* Non-allocating leaf expressions */
    case EXPR_VARIABLE:
        return false;

    case EXPR_THREAD_SYNC:
    case EXPR_SYNC_LIST:
        return true;

    case EXPR_ADDRESS_OF:
        return expr_allocates(expr->as.address_of.operand);

    case EXPR_SIZEOF:
        return false;

    default:
        /* Conservative: unknown expression type → assume allocation */
        return true;
    }
}

/* Check if a statement contains expressions that directly allocate on the arena. */
static bool stmt_allocates(Stmt *stmt)
{
    if (stmt == NULL) return false;

    switch (stmt->type)
    {
    case STMT_EXPR:
        return expr_allocates(stmt->as.expression.expression);

    case STMT_VAR_DECL:
        if (stmt->as.var_decl.type != NULL && type_needs_arena(stmt->as.var_decl.type))
            return true;
        /* Struct locals with handle fields need ARENA_CHILD because
         * code_gen_free_locals emits __release_*_inline__ which frees handles
         * matching __local_arena__. With ARENA_NONE that would be the caller's
         * arena, causing use-after-free on promoted handles. */
        if (stmt->as.var_decl.type != NULL &&
            stmt->as.var_decl.type->kind == TYPE_STRUCT &&
            struct_has_handle_fields(stmt->as.var_decl.type))
            return true;
        if (stmt->as.var_decl.initializer)
            return expr_allocates(stmt->as.var_decl.initializer);
        if (stmt->as.var_decl.mem_qualifier == MEM_AS_REF)
            return true;
        return false;

    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
            return expr_allocates(stmt->as.return_stmt.value);
        return false;

    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            if (stmt_allocates(stmt->as.block.statements[i]))
                return true;
        }
        return false;

    case STMT_IF:
        if (expr_allocates(stmt->as.if_stmt.condition))
            return true;
        if (stmt_allocates(stmt->as.if_stmt.then_branch))
            return true;
        if (stmt->as.if_stmt.else_branch && stmt_allocates(stmt->as.if_stmt.else_branch))
            return true;
        return false;

    case STMT_WHILE:
        if (expr_allocates(stmt->as.while_stmt.condition))
            return true;
        return stmt_allocates(stmt->as.while_stmt.body);

    case STMT_FOR:
        if (stmt->as.for_stmt.initializer && stmt_allocates(stmt->as.for_stmt.initializer))
            return true;
        if (stmt->as.for_stmt.condition && expr_allocates(stmt->as.for_stmt.condition))
            return true;
        if (stmt->as.for_stmt.increment && expr_allocates(stmt->as.for_stmt.increment))
            return true;
        return stmt_allocates(stmt->as.for_stmt.body);

    case STMT_FOR_EACH:
        /* For-each iteration creates element handles */
        return true;

    case STMT_LOCK:
        if (expr_allocates(stmt->as.lock_stmt.lock_expr))
            return true;
        return stmt_allocates(stmt->as.lock_stmt.body);

    case STMT_USING:
        if (expr_allocates(stmt->as.using_stmt.initializer))
            return true;
        return stmt_allocates(stmt->as.using_stmt.body);

    case STMT_FUNCTION:
        /* Nested functions don't affect parent's arena needs */
        return false;

    case STMT_BREAK:
    case STMT_CONTINUE:
    case STMT_IMPORT:
        return false;

    default:
        /* Conservative: unknown statement type → assume allocation */
        return true;
    }
}

ArenaStrategy analyze_arena_strategy(Stmt **body, int body_count, Parameter *params, int param_count)
{
    /* Check parameters for 'as val' with handle types — cloning allocates */
    for (int i = 0; i < param_count; i++)
    {
        if (params[i].mem_qualifier == MEM_AS_VAL)
        {
            if (params[i].type != NULL && type_needs_arena(params[i].type))
                return ARENA_CHILD;
        }
    }

    /* Walk function body for direct allocation expressions */
    for (int i = 0; i < body_count; i++)
    {
        if (stmt_allocates(body[i]))
            return ARENA_CHILD;
    }

    return ARENA_NONE;
}
