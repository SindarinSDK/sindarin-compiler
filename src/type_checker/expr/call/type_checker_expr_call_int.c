/* ============================================================================
 * type_checker_expr_call_int.c - Int Method Type Checking
 * ============================================================================
 * Type checking for int method access (not calls).
 * Returns the function type for the method, or NULL if not an int method.
 * ============================================================================ */

#include "type_checker/expr/call/type_checker_expr_call_int.h"
#include "debug.h"
#include <string.h>

Type *type_check_int_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr;

    if (object_type->kind != TYPE_INT)
    {
        return NULL;
    }

    const char *name = member_name.start;

    /* int.toDouble() -> double */
    if (strcmp(name, "toDouble") == 0)
    {
        Type *double_type = ast_create_primitive_type(table->arena, TYPE_DOUBLE);
        Type *param_types[] = {NULL};
        return ast_create_function_type(table->arena, double_type, param_types, 0);
    }

    /* int.toLong() -> long */
    if (strcmp(name, "toLong") == 0)
    {
        Type *long_type = ast_create_primitive_type(table->arena, TYPE_LONG);
        Type *param_types[] = {NULL};
        return ast_create_function_type(table->arena, long_type, param_types, 0);
    }

    /* int.toUint() -> uint */
    if (strcmp(name, "toUint") == 0)
    {
        Type *uint_type = ast_create_primitive_type(table->arena, TYPE_UINT);
        Type *param_types[] = {NULL};
        return ast_create_function_type(table->arena, uint_type, param_types, 0);
    }

    /* int.toByte() -> byte */
    if (strcmp(name, "toByte") == 0)
    {
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *param_types[] = {NULL};
        return ast_create_function_type(table->arena, byte_type, param_types, 0);
    }

    /* int.toChar() -> char */
    if (strcmp(name, "toChar") == 0)
    {
        Type *char_type = ast_create_primitive_type(table->arena, TYPE_CHAR);
        Type *param_types[] = {NULL};
        return ast_create_function_type(table->arena, char_type, param_types, 0);
    }

    return NULL;
}
