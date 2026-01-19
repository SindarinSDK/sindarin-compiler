#include "code_gen/code_gen_stmt_capture.h"
#include "symbol_table.h"
#include <string.h>

/* Helper to check if type needs capture by reference.
 * This includes primitives (which can be reassigned) and arrays
 * (because push/pop operations return new pointers that must be
 * written back to persist across closure calls). */
static bool needs_capture_by_ref(Type *type)
{
    if (type == NULL) return false;
    switch (type->kind) {
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_DOUBLE:
        case TYPE_BOOL:
        case TYPE_BYTE:
        case TYPE_CHAR:
        case TYPE_ARRAY:
            return true;
        default:
            return false;
    }
}

/* Forward declaration for expression and statement scanning.
 * lambda_depth tracks how many lambda scopes we're nested in - we only
 * capture variables when lambda_depth > 0 (i.e., inside a lambda). */
static void scan_expr_for_captures(CodeGen *gen, Expr *expr, SymbolTable *table, int lambda_depth);
static void scan_stmt_for_captures(CodeGen *gen, Stmt *stmt, SymbolTable *table, int lambda_depth);

/* Add a variable name to the captured primitives list */
static void add_captured_primitive(CodeGen *gen, const char *name)
{
    /* Check if already in list */
    for (int i = 0; i < gen->captured_prim_count; i++)
    {
        if (strcmp(gen->captured_primitives[i], name) == 0)
            return;
    }
    /* Grow array if needed */
    if (gen->captured_prim_count >= gen->captured_prim_capacity)
    {
        int new_cap = gen->captured_prim_capacity == 0 ? 8 : gen->captured_prim_capacity * 2;
        char **new_names = arena_alloc(gen->arena, new_cap * sizeof(char *));
        for (int i = 0; i < gen->captured_prim_count; i++)
        {
            new_names[i] = gen->captured_primitives[i];
        }
        gen->captured_primitives = new_names;
        gen->captured_prim_capacity = new_cap;
    }
    gen->captured_primitives[gen->captured_prim_count] = arena_strdup(gen->arena, name);
    gen->captured_prim_count++;
}

/* Scan an expression to find primitive identifiers that are captured by lambdas.
 * lambda_depth tracks how many lambda scopes we're nested in. We only mark
 * variables as captured when lambda_depth > 0 (i.e., we're inside a lambda). */
static void scan_expr_for_captures(CodeGen *gen, Expr *expr, SymbolTable *table, int lambda_depth)
{
    if (expr == NULL) return;

    switch (expr->type)
    {
    case EXPR_LAMBDA:
    {
        /* Found a lambda - look for outer scope primitives it references */
        LambdaExpr *lambda = &expr->as.lambda;

        /* Create temp scope for lambda params */
        symbol_table_push_scope(table);
        for (int i = 0; i < lambda->param_count; i++)
        {
            symbol_table_add_symbol(table, lambda->params[i].name, lambda->params[i].type);
        }

        /* Now scan the lambda body for identifiers that reference outer scope primitives.
         * Increment lambda_depth so we know we're inside a lambda. */
        /* Scan single expression body */
        if (lambda->body)
        {
            scan_expr_for_captures(gen, lambda->body, table, lambda_depth + 1);
        }
        /* Scan statement body - use full statement scanner to handle all statement types */
        if (lambda->has_stmt_body)
        {
            for (int i = 0; i < lambda->body_stmt_count; i++)
            {
                scan_stmt_for_captures(gen, lambda->body_stmts[i], table, lambda_depth + 1);
            }
        }
        symbol_table_pop_scope(table);
        break;
    }
    case EXPR_VARIABLE:
    {
        /* Only capture variables when we're inside a lambda (lambda_depth > 0) */
        if (lambda_depth > 0)
        {
            /* Check if this identifier is a primitive in outer scope that would be captured */
            char name[256];
            int len = expr->as.variable.name.length < 255 ? expr->as.variable.name.length : 255;
            strncpy(name, expr->as.variable.name.start, len);
            name[len] = '\0';

            Token tok = expr->as.variable.name;
            Symbol *sym = symbol_table_lookup_symbol(table, tok);
            if (sym && sym->kind == SYMBOL_LOCAL && needs_capture_by_ref(sym->type))
            {
                /* This is a local primitive referenced from inside a lambda - it's captured */
                add_captured_primitive(gen, name);
            }
        }
        break;
    }
    case EXPR_BINARY:
        scan_expr_for_captures(gen, expr->as.binary.left, table, lambda_depth);
        scan_expr_for_captures(gen, expr->as.binary.right, table, lambda_depth);
        break;
    case EXPR_UNARY:
        scan_expr_for_captures(gen, expr->as.unary.operand, table, lambda_depth);
        break;
    case EXPR_ASSIGN:
        scan_expr_for_captures(gen, expr->as.assign.value, table, lambda_depth);
        break;
    case EXPR_CALL:
        scan_expr_for_captures(gen, expr->as.call.callee, table, lambda_depth);
        for (int i = 0; i < expr->as.call.arg_count; i++)
            scan_expr_for_captures(gen, expr->as.call.arguments[i], table, lambda_depth);
        break;
    case EXPR_ARRAY:
        for (int i = 0; i < expr->as.array.element_count; i++)
            scan_expr_for_captures(gen, expr->as.array.elements[i], table, lambda_depth);
        break;
    case EXPR_ARRAY_ACCESS:
        scan_expr_for_captures(gen, expr->as.array_access.array, table, lambda_depth);
        scan_expr_for_captures(gen, expr->as.array_access.index, table, lambda_depth);
        break;
    case EXPR_INDEX_ASSIGN:
        scan_expr_for_captures(gen, expr->as.index_assign.array, table, lambda_depth);
        scan_expr_for_captures(gen, expr->as.index_assign.index, table, lambda_depth);
        scan_expr_for_captures(gen, expr->as.index_assign.value, table, lambda_depth);
        break;
    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        scan_expr_for_captures(gen, expr->as.operand, table, lambda_depth);
        break;
    case EXPR_INTERPOLATED:
        for (int i = 0; i < expr->as.interpol.part_count; i++)
            scan_expr_for_captures(gen, expr->as.interpol.parts[i], table, lambda_depth);
        break;
    case EXPR_MEMBER:
        scan_expr_for_captures(gen, expr->as.member.object, table, lambda_depth);
        break;
    case EXPR_ARRAY_SLICE:
        scan_expr_for_captures(gen, expr->as.array_slice.array, table, lambda_depth);
        if (expr->as.array_slice.start) scan_expr_for_captures(gen, expr->as.array_slice.start, table, lambda_depth);
        if (expr->as.array_slice.end) scan_expr_for_captures(gen, expr->as.array_slice.end, table, lambda_depth);
        if (expr->as.array_slice.step) scan_expr_for_captures(gen, expr->as.array_slice.step, table, lambda_depth);
        break;
    case EXPR_RANGE:
        scan_expr_for_captures(gen, expr->as.range.start, table, lambda_depth);
        scan_expr_for_captures(gen, expr->as.range.end, table, lambda_depth);
        break;
    case EXPR_SPREAD:
        scan_expr_for_captures(gen, expr->as.spread.array, table, lambda_depth);
        break;
    case EXPR_STATIC_CALL:
        for (int i = 0; i < expr->as.static_call.arg_count; i++)
            scan_expr_for_captures(gen, expr->as.static_call.arguments[i], table, lambda_depth);
        break;
    default:
        break;
    }
}

/* Scan a statement for lambda expressions and their captures.
 * lambda_depth tracks how many lambda scopes we're nested in. */
static void scan_stmt_for_captures(CodeGen *gen, Stmt *stmt, SymbolTable *table, int lambda_depth)
{
    if (stmt == NULL) return;

    switch (stmt->type)
    {
    case STMT_VAR_DECL:
        /* First add this variable to scope so nested lambdas can see it */
        symbol_table_add_symbol_full(table, stmt->as.var_decl.name, stmt->as.var_decl.type,
                                     SYMBOL_LOCAL, stmt->as.var_decl.mem_qualifier);
        /* Then scan the initializer for lambda captures */
        if (stmt->as.var_decl.initializer)
            scan_expr_for_captures(gen, stmt->as.var_decl.initializer, table, lambda_depth);
        break;
    case STMT_EXPR:
        scan_expr_for_captures(gen, stmt->as.expression.expression, table, lambda_depth);
        break;
    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
            scan_expr_for_captures(gen, stmt->as.return_stmt.value, table, lambda_depth);
        break;
    case STMT_BLOCK:
        symbol_table_push_scope(table);
        for (int i = 0; i < stmt->as.block.count; i++)
            scan_stmt_for_captures(gen, stmt->as.block.statements[i], table, lambda_depth);
        symbol_table_pop_scope(table);
        break;
    case STMT_IF:
        scan_expr_for_captures(gen, stmt->as.if_stmt.condition, table, lambda_depth);
        scan_stmt_for_captures(gen, stmt->as.if_stmt.then_branch, table, lambda_depth);
        if (stmt->as.if_stmt.else_branch)
            scan_stmt_for_captures(gen, stmt->as.if_stmt.else_branch, table, lambda_depth);
        break;
    case STMT_WHILE:
        scan_expr_for_captures(gen, stmt->as.while_stmt.condition, table, lambda_depth);
        scan_stmt_for_captures(gen, stmt->as.while_stmt.body, table, lambda_depth);
        break;
    case STMT_FOR:
        symbol_table_push_scope(table);
        if (stmt->as.for_stmt.initializer)
            scan_stmt_for_captures(gen, stmt->as.for_stmt.initializer, table, lambda_depth);
        if (stmt->as.for_stmt.condition)
            scan_expr_for_captures(gen, stmt->as.for_stmt.condition, table, lambda_depth);
        if (stmt->as.for_stmt.increment)
            scan_expr_for_captures(gen, stmt->as.for_stmt.increment, table, lambda_depth);
        scan_stmt_for_captures(gen, stmt->as.for_stmt.body, table, lambda_depth);
        symbol_table_pop_scope(table);
        break;
    case STMT_FOR_EACH:
        symbol_table_push_scope(table);
        /* Add loop variable - get element type from iterable's expr_type */
        scan_expr_for_captures(gen, stmt->as.for_each_stmt.iterable, table, lambda_depth);
        if (stmt->as.for_each_stmt.iterable->expr_type &&
            stmt->as.for_each_stmt.iterable->expr_type->kind == TYPE_ARRAY)
        {
            Type *elem_type = stmt->as.for_each_stmt.iterable->expr_type->as.array.element_type;
            symbol_table_add_symbol(table, stmt->as.for_each_stmt.var_name, elem_type);
        }
        scan_stmt_for_captures(gen, stmt->as.for_each_stmt.body, table, lambda_depth);
        symbol_table_pop_scope(table);
        break;
    case STMT_LOCK:
        scan_expr_for_captures(gen, stmt->as.lock_stmt.lock_expr, table, lambda_depth);
        scan_stmt_for_captures(gen, stmt->as.lock_stmt.body, table, lambda_depth);
        break;
    default:
        break;
    }
}

/* Pre-pass to scan a function body for primitives captured by closures */
void code_gen_scan_captured_primitives(CodeGen *gen, Stmt **stmts, int stmt_count)
{
    /* Clear any existing captured primitives */
    code_gen_clear_captured_primitives(gen);

    /* Create a temporary symbol table scope for scanning */
    symbol_table_push_scope(gen->symbol_table);

    /* Start with lambda_depth = 0 (not inside any lambda) */
    for (int i = 0; i < stmt_count; i++)
    {
        scan_stmt_for_captures(gen, stmts[i], gen->symbol_table, 0);
    }

    symbol_table_pop_scope(gen->symbol_table);
}

/* Check if a variable name is a captured primitive */
bool code_gen_is_captured_primitive(CodeGen *gen, const char *name)
{
    for (int i = 0; i < gen->captured_prim_count; i++)
    {
        if (strcmp(gen->captured_primitives[i], name) == 0)
            return true;
    }
    return false;
}

/* Clear the captured primitives list */
void code_gen_clear_captured_primitives(CodeGen *gen)
{
    gen->captured_prim_count = 0;
}

/* Push arena name onto the private block arena stack */
void push_arena_to_stack(CodeGen *gen, const char *arena_name)
{
    /* Grow stack if needed */
    if (gen->arena_stack_depth >= gen->arena_stack_capacity)
    {
        int new_cap = gen->arena_stack_capacity == 0 ? 4 : gen->arena_stack_capacity * 2;
        char **new_stack = arena_alloc(gen->arena, new_cap * sizeof(char *));
        for (int i = 0; i < gen->arena_stack_depth; i++)
        {
            new_stack[i] = gen->arena_stack[i];
        }
        gen->arena_stack = new_stack;
        gen->arena_stack_capacity = new_cap;
    }
    gen->arena_stack[gen->arena_stack_depth] = arena_strdup(gen->arena, arena_name);
    gen->arena_stack_depth++;
}

/* Pop arena name from the private block arena stack */
const char *pop_arena_from_stack(CodeGen *gen)
{
    if (gen->arena_stack_depth <= 0)
    {
        return NULL;
    }
    gen->arena_stack_depth--;
    return gen->arena_stack[gen->arena_stack_depth];
}
