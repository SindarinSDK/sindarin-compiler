#include "type_checker/expr/type_checker_expr_basic.h"
#include "type_checker/util/type_checker_util.h"
#include "debug.h"
#include <string.h>

Type *type_check_literal(Expr *expr, SymbolTable *table)
{
    (void)table;
    DEBUG_VERBOSE("Type checking literal expression");
    return expr->as.literal.type;
}

Type *type_check_variable(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking variable: %.*s", expr->as.variable.name.length, expr->as.variable.name.start);
    Symbol *sym = symbol_table_lookup_symbol(table, expr->as.variable.name);
    if (sym == NULL)
    {
        undefined_variable_error(&expr->as.variable.name, table);
        return NULL;
    }
    if (sym->type == NULL)
    {
        /* Check if this is a namespace being used incorrectly as a variable */
        if (sym->is_namespace)
        {
            char name_str[128];
            int name_len = expr->as.variable.name.length < 127 ? expr->as.variable.name.length : 127;
            memcpy(name_str, expr->as.variable.name.start, name_len);
            name_str[name_len] = '\0';

            char msg[256];
            snprintf(msg, sizeof(msg), "'%s' is a namespace, not a variable", name_str);
            type_error_with_suggestion(&expr->as.variable.name, msg,
                "Use namespace.symbol to access symbols in a namespace");
        }
        else
        {
            type_error(&expr->as.variable.name, "Symbol has no type");
        }
        return NULL;
    }

    /* Check if variable is a pending thread (not yet synchronized) */
    if (symbol_table_is_pending(sym))
    {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "Cannot access pending thread variable '%.*s' before synchronization (use %.*s! to sync)",
                 expr->as.variable.name.length, expr->as.variable.name.start,
                 expr->as.variable.name.length, expr->as.variable.name.start);
        type_error(&expr->as.variable.name, msg);
        return NULL;
    }

    /* Resolve forward references for struct types */
    Type *result_type = sym->type;
    if (result_type->kind == TYPE_STRUCT)
    {
        result_type = resolve_struct_forward_reference(result_type, table);
    }

    DEBUG_VERBOSE("Variable type found: %d", result_type->kind);
    return result_type;
}
