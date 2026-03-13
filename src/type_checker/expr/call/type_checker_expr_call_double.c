/* ============================================================================
 * type_checker_expr_call_double.c - Double Method Type Checking
 * ============================================================================ */

#include "type_checker/expr/call/type_checker_expr_call_double.h"
#include "debug.h"
#include <string.h>

Type *type_check_double_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr;

    if (object_type->kind != TYPE_DOUBLE)
    {
        return NULL;
    }

    const char *name = member_name.start;

    /* double.toInt() -> int */
    if (strcmp(name, "toInt") == 0)
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* double.toLong() -> long */
    if (strcmp(name, "toLong") == 0)
    {
        Type *long_type = ast_create_primitive_type(table->arena, TYPE_LONG);
        Type *param_types[] = {NULL};
        return ast_create_function_type(table->arena, long_type, param_types, 0);
    }

    return NULL;
}
