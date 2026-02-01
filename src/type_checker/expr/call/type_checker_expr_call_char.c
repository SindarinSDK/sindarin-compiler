/* ============================================================================
 * type_checker_expr_call_char.c - Char Method Type Checking
 * ============================================================================
 * Type checking for char method access (not calls).
 * Returns the function type for the method, or NULL if not a char method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

#include "type_checker/expr/call/type_checker_expr_call_char.h"
#include "debug.h"
#include <string.h>

/* ============================================================================
 * Char Method Type Checking
 * ============================================================================ */

Type *type_check_char_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle char types */
    if (object_type->kind != TYPE_CHAR)
    {
        return NULL;
    }

    const char *name = member_name.start;

    /* char.toString() -> str */
    if (strcmp(name, "toString") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for char toString method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }

    /* char.toUpper() -> char */
    if (strcmp(name, "toUpper") == 0)
    {
        Type *char_type = ast_create_primitive_type(table->arena, TYPE_CHAR);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for char toUpper method");
        return ast_create_function_type(table->arena, char_type, param_types, 0);
    }

    /* char.toLower() -> char */
    if (strcmp(name, "toLower") == 0)
    {
        Type *char_type = ast_create_primitive_type(table->arena, TYPE_CHAR);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for char toLower method");
        return ast_create_function_type(table->arena, char_type, param_types, 0);
    }

    /* char.toInt() -> int (returns ASCII value) */
    if (strcmp(name, "toInt") == 0)
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for char toInt method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* char.isDigit() -> bool */
    if (strcmp(name, "isDigit") == 0)
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for char isDigit method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* char.isAlpha() -> bool */
    if (strcmp(name, "isAlpha") == 0)
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for char isAlpha method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* char.isWhitespace() -> bool */
    if (strcmp(name, "isWhitespace") == 0)
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for char isWhitespace method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* char.isAlnum() -> bool (alphanumeric check) */
    if (strcmp(name, "isAlnum") == 0)
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for char isAlnum method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* Not a recognized char method */
    return NULL;
}
