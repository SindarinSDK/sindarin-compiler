/* ============================================================================
 * type_checker_expr_call_bool.c - Bool Method Type Checking
 * ============================================================================ */

#include "type_checker/expr/call/type_checker_expr_call_bool.h"
#include "debug.h"
#include <string.h>

Type *type_check_bool_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr;

    if (object_type->kind != TYPE_BOOL)
    {
        return NULL;
    }

    const char *name = member_name.start;

    /* bool.toInt() -> int (true=1, false=0) */
    if (strcmp(name, "toInt") == 0)
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    return NULL;
}
