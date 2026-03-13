/* ============================================================================
 * type_checker_expr_call_long.c - Long Method Type Checking
 * ============================================================================ */

#include "type_checker/expr/call/type_checker_expr_call_long.h"
#include "debug.h"
#include <string.h>

Type *type_check_long_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr;

    if (object_type->kind != TYPE_LONG)
    {
        return NULL;
    }

    const char *name = member_name.start;

    /* long.toInt() -> int */
    if (strcmp(name, "toInt") == 0)
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* long.toDouble() -> double */
    if (strcmp(name, "toDouble") == 0)
    {
        Type *double_type = ast_create_primitive_type(table->arena, TYPE_DOUBLE);
        Type *param_types[] = {NULL};
        return ast_create_function_type(table->arena, double_type, param_types, 0);
    }

    return NULL;
}
