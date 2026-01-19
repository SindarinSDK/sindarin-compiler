/* ============================================================================
 * type_checker_expr_call_string.c - String Method Type Checking
 * ============================================================================
 * Type checking for string method access (not calls).
 * Returns the function type for the method, or NULL if not a string method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

#include "type_checker/type_checker_expr_call_string.h"
#include "debug.h"
#include <string.h>

/* ============================================================================
 * String Method Type Checking
 * ============================================================================ */

Type *type_check_string_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle string types */
    if (object_type->kind != TYPE_STRING)
    {
        return NULL;
    }

    const char *name = member_name.start;

    /* string.length property - returns int directly */
    if (strcmp(name, "length") == 0)
    {
        DEBUG_VERBOSE("Returning INT type for string length access");
        return ast_create_primitive_type(table->arena, TYPE_INT);
    }

    /* string.substring(start, end) -> str */
    if (strcmp(name, "substring") == 0)
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[2] = {int_type, int_type};
        DEBUG_VERBOSE("Returning function type for string substring method");
        return ast_create_function_type(table->arena, string_type, param_types, 2);
    }

    /* string.regionEquals(start, length, other) -> bool */
    if (strcmp(name, "regionEquals") == 0)
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[3] = {int_type, int_type, string_type};
        DEBUG_VERBOSE("Returning function type for string regionEquals method");
        return ast_create_function_type(table->arena, bool_type, param_types, 3);
    }

    /* string.indexOf(substr) -> int */
    if (strcmp(name, "indexOf") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for string indexOf method");
        return ast_create_function_type(table->arena, int_type, param_types, 1);
    }

    /* string.split(delimiter) -> str[] */
    if (strcmp(name, "split") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *str_array_type = ast_create_array_type(table->arena, string_type);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for string split method");
        return ast_create_function_type(table->arena, str_array_type, param_types, 1);
    }

    /* string.trim() -> str */
    if (strcmp(name, "trim") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string trim method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }

    /* string.toUpper() -> str */
    if (strcmp(name, "toUpper") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string toUpper method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }

    /* string.toLower() -> str */
    if (strcmp(name, "toLower") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string toLower method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }

    /* string.startsWith(prefix) -> bool */
    if (strcmp(name, "startsWith") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for string startsWith method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* string.endsWith(suffix) -> bool */
    if (strcmp(name, "endsWith") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for string endsWith method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* string.contains(substr) -> bool */
    if (strcmp(name, "contains") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for string contains method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* string.replace(old, new) -> str */
    if (strcmp(name, "replace") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[2] = {string_type, string_type};
        DEBUG_VERBOSE("Returning function type for string replace method");
        return ast_create_function_type(table->arena, string_type, param_types, 2);
    }

    /* string.charAt(index) -> char */
    if (strcmp(name, "charAt") == 0)
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *char_type = ast_create_primitive_type(table->arena, TYPE_CHAR);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for string charAt method");
        return ast_create_function_type(table->arena, char_type, param_types, 1);
    }

    /* string.toBytes() -> byte[] (UTF-8 encoding) */
    if (strcmp(name, "toBytes") == 0)
    {
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *byte_array_type = ast_create_array_type(table->arena, byte_type);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string toBytes method");
        return ast_create_function_type(table->arena, byte_array_type, param_types, 0);
    }

    /* string.splitWhitespace() -> str[] */
    if (strcmp(name, "splitWhitespace") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *str_array_type = ast_create_array_type(table->arena, string_type);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string splitWhitespace method");
        return ast_create_function_type(table->arena, str_array_type, param_types, 0);
    }

    /* string.splitLines() -> str[] */
    if (strcmp(name, "splitLines") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *str_array_type = ast_create_array_type(table->arena, string_type);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string splitLines method");
        return ast_create_function_type(table->arena, str_array_type, param_types, 0);
    }

    /* string.isBlank() -> bool */
    if (strcmp(name, "isBlank") == 0)
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string isBlank method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* string.append(other) -> str */
    if (strcmp(name, "append") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for string append method");
        return ast_create_function_type(table->arena, string_type, param_types, 1);
    }

    /* Not a string method */
    return NULL;
}
