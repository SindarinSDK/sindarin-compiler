#include "type_checker/type_checker_stmt_var.h"
#include "type_checker/type_checker_stmt_var_util.h"
#include "type_checker/type_checker_util.h"
#include "type_checker/type_checker_expr.h"
#include "symbol_table/symbol_table_core.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

/* Reserved keyword table for namespace validation */
static const char *reserved_keywords[] = {
    "fn", "var", "return", "if", "else", "for", "while", "break", "continue",
    "in", "import", "nil", "int", "long", "double", "char", "str", "bool",
    "byte", "void", "shared", "private", "as", "val", "ref", "true", "false",
    "native",
    NULL
};

const char *is_reserved_keyword(Token token)
{
    for (int i = 0; reserved_keywords[i] != NULL; i++)
    {
        const char *keyword = reserved_keywords[i];
        int keyword_len = strlen(keyword);
        if (token.length == keyword_len &&
            memcmp(token.start, keyword, keyword_len) == 0)
        {
            return keyword;
        }
    }
    return NULL;
}

void infer_lambda_types(Expr *lambda_expr, Type *func_type)
{
    if (lambda_expr == NULL || lambda_expr->type != EXPR_LAMBDA)
        return;
    if (func_type == NULL || func_type->kind != TYPE_FUNCTION)
        return;

    LambdaExpr *lambda = &lambda_expr->as.lambda;

    /* Infer is_native from the function type FIRST, before checking parameter count.
     * This ensures the lambda is marked as native even if signatures don't match,
     * which enables better error messages for native lambda signature mismatches. */
    if (func_type->as.function.is_native && !lambda->is_native)
    {
        lambda->is_native = true;
        DEBUG_VERBOSE("Inferred is_native from function type");
    }

    /* Check parameter count matches */
    if (lambda->param_count != func_type->as.function.param_count)
    {
        DEBUG_VERBOSE("Lambda param count %d doesn't match function type param count %d",
                      lambda->param_count, func_type->as.function.param_count);
        return;
    }

    /* Infer missing parameter types */
    for (int i = 0; i < lambda->param_count; i++)
    {
        if (lambda->params[i].type == NULL)
        {
            lambda->params[i].type = func_type->as.function.param_types[i];
            DEBUG_VERBOSE("Inferred parameter %d type from function type", i);
        }
    }

    /* Infer missing return type */
    if (lambda->return_type == NULL)
    {
        lambda->return_type = func_type->as.function.return_type;
        DEBUG_VERBOSE("Inferred return type from function type");
    }
}

void type_check_var_decl(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    (void)return_type;
    DEBUG_VERBOSE("Type checking variable declaration: %.*s",
                  stmt->as.var_decl.name.length, stmt->as.var_decl.name.start);

    /* Check for redeclaration in the current scope */
    Symbol *existing = symbol_table_lookup_symbol_current(table, stmt->as.var_decl.name);
    if (existing != NULL)
    {
        type_error(&stmt->as.var_decl.name, "Variable is already declared in this scope");
        return;
    }

    Type *decl_type = stmt->as.var_decl.type;
    Type *init_type = NULL;
    bool added_for_recursion = false;

    if (stmt->as.var_decl.initializer)
    {
        /* If initializer is a lambda with missing types, infer from declared type */
        if (stmt->as.var_decl.initializer->type == EXPR_LAMBDA &&
            decl_type != NULL && decl_type->kind == TYPE_FUNCTION)
        {
            infer_lambda_types(stmt->as.var_decl.initializer, decl_type);

            /* For recursive lambdas: Add the variable to scope BEFORE type-checking */
            symbol_table_add_symbol_with_kind(table, stmt->as.var_decl.name, decl_type, SYMBOL_LOCAL);
            Symbol *sym = symbol_table_lookup_symbol_current(table, stmt->as.var_decl.name);
            if (sym != NULL)
            {
                sym->is_function = true;
            }
            added_for_recursion = true;
        }

        init_type = type_check_expr(stmt->as.var_decl.initializer, table);
        if (init_type == NULL)
        {
            Type *fallback = decl_type ? decl_type : ast_create_primitive_type(table->arena, TYPE_NIL);
            symbol_table_add_symbol_with_kind(table, stmt->as.var_decl.name, fallback, SYMBOL_LOCAL);
            return;
        }

        /* Void thread spawns cannot be assigned to variables */
        if (stmt->as.var_decl.initializer->type == EXPR_THREAD_SPAWN &&
            init_type->kind == TYPE_VOID)
        {
            type_error(&stmt->as.var_decl.name, "Cannot assign void thread spawn to variable");
            return;
        }

        /* Apply array type coercions */
        apply_array_coercion(stmt, decl_type, &init_type);
    }

    /* Type inference: if no declared type, infer from initializer */
    if (decl_type == NULL)
    {
        if (init_type == NULL)
        {
            type_error(&stmt->as.var_decl.name, "Cannot infer type without initializer");
            decl_type = ast_create_primitive_type(table->arena, TYPE_NIL);
        }
        else
        {
            decl_type = init_type;
            stmt->as.var_decl.type = decl_type;
        }
    }

    /* Reject pointer variable declarations in non-native functions */
    if (decl_type && decl_type->kind == TYPE_POINTER && !native_context_is_active())
    {
        type_error(&stmt->as.var_decl.name,
                   "Pointer variables can only be declared in native functions");
    }

    /* Reject pointer return values from native functions in non-native context */
    if (init_type && init_type->kind == TYPE_POINTER && !native_context_is_active())
    {
        type_error(&stmt->as.var_decl.name,
                   "Pointer types not allowed in non-native functions, use 'as val'");
    }

    /* Validate memory qualifier usage */
    MemoryQualifier mem_qual = stmt->as.var_decl.mem_qualifier;
    if (mem_qual == MEM_AS_REF)
    {
        if (!is_primitive_type(decl_type))
        {
            type_error(&stmt->as.var_decl.name, "'as ref' can only be used with primitive types");
        }
    }
    else if (mem_qual == MEM_AS_VAL)
    {
        if (is_primitive_type(decl_type))
        {
            DEBUG_VERBOSE("Warning: 'as val' on primitive type has no effect");
        }
    }

    /* Only add symbol if we didn't already add it for recursive lambda support */
    if (!added_for_recursion)
    {
        symbol_table_add_symbol_with_kind(table, stmt->as.var_decl.name, decl_type, SYMBOL_LOCAL);
    }

    /* Handle sync modifier */
    if (stmt->as.var_decl.sync_modifier == SYNC_ATOMIC)
    {
        if (decl_type->kind != TYPE_INT && decl_type->kind != TYPE_LONG &&
            decl_type->kind != TYPE_INT32 && decl_type->kind != TYPE_UINT &&
            decl_type->kind != TYPE_UINT32 && decl_type->kind != TYPE_BYTE &&
            decl_type->kind != TYPE_CHAR)
        {
            type_error(&stmt->as.var_decl.name,
                       "sync modifier is only allowed on integer types (int, long, int32, uint, uint32, byte, char)");
        }
        else
        {
            Symbol *symbol = symbol_table_lookup_symbol_current(table, stmt->as.var_decl.name);
            if (symbol != NULL)
            {
                symbol->sync_mod = SYNC_ATOMIC;
            }
        }
    }

    /* Handle static modifier */
    if (stmt->as.var_decl.is_static)
    {
        Symbol *symbol = symbol_table_lookup_symbol_current(table, stmt->as.var_decl.name);
        if (symbol != NULL)
        {
            symbol->is_static = true;
        }
    }

    /* Check: nil can only be assigned to reference/pointer types */
    if (init_type && init_type->kind == TYPE_NIL &&
        decl_type->kind != TYPE_POINTER &&
        decl_type->kind != TYPE_STRING &&
        decl_type->kind != TYPE_ARRAY &&
        decl_type->kind != TYPE_ANY &&
        decl_type->kind != TYPE_FUNCTION)
    {
        type_error(&stmt->as.var_decl.name, "'nil' can only be assigned to reference or pointer types");
        return;
    }

    /* Check type compatibility */
    bool types_compatible = check_var_type_compatibility(decl_type, init_type, stmt);

    if (init_type && !types_compatible)
    {
        if (stmt->as.var_decl.initializer &&
            stmt->as.var_decl.initializer->type == EXPR_THREAD_SPAWN)
        {
            type_error(&stmt->as.var_decl.name,
                       "Thread spawn return type does not match variable type");
        }
        else if (stmt->as.var_decl.initializer &&
                 stmt->as.var_decl.initializer->type == EXPR_LAMBDA &&
                 stmt->as.var_decl.initializer->as.lambda.is_native &&
                 decl_type->kind == TYPE_FUNCTION &&
                 decl_type->as.function.is_native)
        {
            if (init_type->kind == TYPE_FUNCTION &&
                init_type->as.function.param_count != decl_type->as.function.param_count)
            {
                type_error(&stmt->as.var_decl.name,
                           "Native lambda parameter count does not match callback type");
            }
            else if (init_type->kind == TYPE_FUNCTION &&
                     !ast_type_equals(init_type->as.function.return_type,
                                      decl_type->as.function.return_type))
            {
                type_error(&stmt->as.var_decl.name,
                           "Native lambda return type does not match callback type");
            }
            else
            {
                type_error(&stmt->as.var_decl.name,
                           "Native lambda signature does not match callback type");
            }
        }
        else
        {
            type_error(&stmt->as.var_decl.name, "Initializer type does not match variable type");
        }
    }

    /* Mark variable as pending if initialized with a thread spawn (non-void) */
    if (stmt->as.var_decl.initializer &&
        stmt->as.var_decl.initializer->type == EXPR_THREAD_SPAWN &&
        init_type && init_type->kind != TYPE_VOID)
    {
        Symbol *sym = symbol_table_lookup_symbol(table, stmt->as.var_decl.name);
        if (sym != NULL)
        {
            symbol_table_mark_pending(sym);
        }
    }
}
