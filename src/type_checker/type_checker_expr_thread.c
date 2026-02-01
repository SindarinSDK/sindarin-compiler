#include "type_checker/type_checker_expr_thread.h"
#include "type_checker/type_checker_expr.h"
#include "type_checker/type_checker_util.h"
#include "symbol_table/symbol_table_thread.h"
#include "debug.h"

Type *type_check_thread_spawn(Expr *expr, SymbolTable *table)
{
    Expr *call = expr->as.thread_spawn.call;

    /* Validate that the spawn expression is a function call or static call */
    if (call == NULL || (call->type != EXPR_CALL && call->type != EXPR_STATIC_CALL))
    {
        type_error(expr->token, "Thread spawn requires function call");
        return NULL;
    }

    /* Handle static method calls like &Process.run(...) */
    if (call->type == EXPR_STATIC_CALL)
    {
        /* Type check the static call expression */
        Type *result_type = type_check_expr(call, table);
        if (result_type == NULL)
        {
            return NULL;
        }

        /* Static methods use default modifier (no shared/private support) */
        expr->as.thread_spawn.modifier = FUNC_DEFAULT;

        DEBUG_VERBOSE("Thread spawn static call type checked, return type: %d", result_type->kind);
        return result_type;
    }

    /* Regular function call handling below */

    /* Type check the call expression to get the function type */
    Expr *callee = call->as.call.callee;
    Type *func_type = type_check_expr(callee, table);
    if (func_type == NULL)
    {
        type_error(expr->token, "Cannot resolve function in thread spawn");
        return NULL;
    }

    /* Validate that the callee is a function type */
    if (func_type->kind != TYPE_FUNCTION)
    {
        type_error(expr->token, "Thread spawn requires function call");
        return NULL;
    }

    /* Look up the function symbol to get its DECLARED modifier (shared/private/default).
     * The symbol stores both func_mod (effective modifier for code gen) and
     * declared_func_mod (what the user wrote). For thread spawning, we need the
     * declared modifier because:
     * - Default mode: thread gets own arena, results are promoted to caller's arena
     * - Shared mode: thread uses caller's arena directly, no promotion needed
     * Functions that are "implicitly shared" (return heap types) use declared=default
     * but effective=shared, so they spawn in default mode with result promotion. */
    FunctionModifier func_modifier = FUNC_DEFAULT;
    if (callee->type == EXPR_VARIABLE)
    {
        Symbol *func_sym = symbol_table_lookup_symbol(table, callee->as.variable.name);
        if (func_sym != NULL && func_sym->is_function)
        {
            /* Use declared modifier for thread spawn mode, not effective modifier */
            func_modifier = func_sym->declared_func_mod;
            DEBUG_VERBOSE("Thread spawn function '%.*s' has declared modifier: %d (effective: %d)",
                          callee->as.variable.name.length,
                          callee->as.variable.name.start,
                          func_modifier, func_sym->func_mod);
        }
    }
    /* Store the function modifier in the spawn expression for code generation */
    expr->as.thread_spawn.modifier = func_modifier;

    /* Also type check the full call expression to validate arguments */
    Type *result_type = type_check_expr(call, table);
    if (result_type == NULL)
    {
        return NULL;
    }

    /* Extract return type from function type */
    Type *return_type = func_type->as.function.return_type;

    /* Private functions can only return primitive types.
     * This is enforced because private functions have isolated arenas that
     * are freed immediately after execution - only primitives can escape. */
    if (func_modifier == FUNC_PRIVATE && return_type != NULL)
    {
        bool is_primitive = (return_type->kind == TYPE_INT ||
                             return_type->kind == TYPE_LONG ||
                             return_type->kind == TYPE_DOUBLE ||
                             return_type->kind == TYPE_BOOL ||
                             return_type->kind == TYPE_BYTE ||
                             return_type->kind == TYPE_CHAR ||
                             return_type->kind == TYPE_VOID);
        if (!is_primitive)
        {
            type_error(expr->token, "Private function can only return primitive types");
            return NULL;
        }
    }

    DEBUG_VERBOSE("Thread spawn type checked, return type: %d", return_type->kind);

    return return_type;
}

/* Thread sync expression type checking - var! or &fn()! or [r1,r2,r3]! */
Type *type_check_thread_sync(Expr *expr, SymbolTable *table)
{
    Expr *handle = expr->as.thread_sync.handle;
    bool is_array = expr->as.thread_sync.is_array;

    if (handle == NULL)
    {
        type_error(expr->token, "Thread sync requires handle expression");
        return NULL;
    }

    /* Check for sync list pattern: [r1, r2, r3]! */
    if (is_array)
    {
        /* Validate handle is a sync list expression */
        if (handle->type != EXPR_SYNC_LIST)
        {
            type_error(expr->token, "Multi-sync requires [var1, var2, ...]! syntax");
            return NULL;
        }

        SyncListExpr *sync_list = &handle->as.sync_list;

        /* First pass: validate all elements are valid thread variables.
         * We validate all before syncing any to ensure atomic-like behavior. */
        for (int i = 0; i < sync_list->element_count; i++)
        {
            Expr *elem = sync_list->elements[i];
            if (elem == NULL)
            {
                type_error(expr->token, "Sync list element cannot be null");
                return NULL;
            }

            if (elem->type != EXPR_VARIABLE)
            {
                type_error(expr->token, "Sync list elements must be thread handle variables");
                return NULL;
            }

            Symbol *sym = symbol_table_lookup_symbol(table, elem->as.variable.name);
            if (sym == NULL)
            {
                type_error(expr->token, "Cannot sync unknown variable in sync list");
                return NULL;
            }

            /* Check thread state - must be either pending or already synchronized.
             * Normal state (never spawned) is an error. */
            if (sym->thread_state == THREAD_STATE_NORMAL)
            {
                type_error(expr->token, "Sync list element is not a thread variable");
                return NULL;
            }
        }

        /* Second pass: sync all pending variables.
         * Skip already synchronized ones (handles mixed states gracefully). */
        int synced_count = 0;
        for (int i = 0; i < sync_list->element_count; i++)
        {
            Expr *elem = sync_list->elements[i];
            Symbol *sym = symbol_table_lookup_symbol(table, elem->as.variable.name);

            /* Only sync if still pending - already synchronized is OK */
            if (symbol_table_is_pending(sym))
            {
                /* Sync the variable - transitions from pending to synchronized */
                symbol_table_sync_variable(table, elem->as.variable.name);
                synced_count++;
            }
            /* Already synchronized variables are silently accepted */
        }

        DEBUG_VERBOSE("Sync list type checked with %d elements, %d newly synced, returning void",
                      sync_list->element_count, synced_count);

        /* Sync list returns void - no single return value */
        return ast_create_primitive_type(table->arena, TYPE_VOID);
    }

    /* Check for inline spawn-sync pattern: &fn()! */
    if (handle->type == EXPR_THREAD_SPAWN)
    {
        /* Type check the spawn expression - this validates the call */
        Type *spawn_type = type_check_thread_spawn(handle, table);
        if (spawn_type == NULL)
        {
            return NULL;
        }

        /* For inline spawn-sync, we don't mark anything as pending.
         * The thread is spawned and immediately joined, so no variable
         * is left in pending state. Just return the synchronized type. */
        DEBUG_VERBOSE("Inline spawn-sync type checked, return type: %d", spawn_type->kind);
        return spawn_type;
    }

    /* Regular sync on a variable: var! */
    if (handle->type == EXPR_VARIABLE)
    {
        Symbol *sym = symbol_table_lookup_symbol(table, handle->as.variable.name);
        if (sym == NULL)
        {
            type_error(expr->token, "Cannot sync unknown variable");
            return NULL;
        }

        /* Check if the variable's type supports thread spawn.
         * We allow sync on any variable of a thread-compatible type
         * (primitives, structs, strings, arrays) even if it's not currently
         * marked as pending. This enables conditional thread spawns:
         *   var h: Result = default_value
         *   if condition =>
         *       h = &compute()  // may or may not execute
         *   h!  // sync if pending, otherwise return current value
         * At runtime, the code checks if __var_pending__ is NULL. */
        Type *var_type = sym->type;
        bool is_thread_compatible = (var_type != NULL &&
            (var_type->kind == TYPE_INT ||
             var_type->kind == TYPE_LONG ||
             var_type->kind == TYPE_DOUBLE ||
             var_type->kind == TYPE_BOOL ||
             var_type->kind == TYPE_BYTE ||
             var_type->kind == TYPE_CHAR ||
             var_type->kind == TYPE_STRING ||
             var_type->kind == TYPE_ARRAY ||
             var_type->kind == TYPE_STRUCT));

        if (!is_thread_compatible)
        {
            type_error(expr->token, "Cannot sync variable of this type");
            return NULL;
        }

        /* If variable is definitely pending, mark it as synchronized.
         * If it's not pending (conditional spawn path), that's OK -
         * the runtime will check and handle it. */
        if (symbol_table_is_pending(sym))
        {
            symbol_table_sync_variable(table, handle->as.variable.name);
        }

        DEBUG_VERBOSE("Variable sync type checked, return type: %d", sym->type->kind);
        return sym->type;
    }

    type_error(expr->token, "Sync requires thread handle variable");
    return NULL;
}
