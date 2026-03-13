/* ============================================================================
 * type_checker_expr_call_byte.c - Byte Method Type Checking
 * ============================================================================ */

#include "type_checker/expr/call/type_checker_expr_call_byte.h"
#include "debug.h"
#include <string.h>

Type *type_check_byte_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr;

    if (object_type->kind != TYPE_BYTE)
    {
        return NULL;
    }

    const char *name = member_name.start;

    /* byte.toInt() -> int */
    if (strcmp(name, "toInt") == 0)
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* byte.toChar() -> char */
    if (strcmp(name, "toChar") == 0)
    {
        Type *char_type = ast_create_primitive_type(table->arena, TYPE_CHAR);
        Type *param_types[] = {NULL};
        return ast_create_function_type(table->arena, char_type, param_types, 0);
    }

    return NULL;
}
