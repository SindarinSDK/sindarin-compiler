/* ============================================================================
 * type_checker_expr_call_array.c - Array Method Type Checking
 * ============================================================================
 * Type checking for array method access (not calls).
 * Returns the function type for the method, or NULL if not an array method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

#include "type_checker/type_checker_expr_call_array.h"
#include "debug.h"
#include <string.h>

/* ============================================================================
 * Array Method Type Checking
 * ============================================================================ */

Type *type_check_array_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle array types */
    if (object_type->kind != TYPE_ARRAY)
    {
        return NULL;
    }

    const char *name = member_name.start;

    /* array.length property - returns int directly */
    if (strcmp(name, "length") == 0)
    {
        DEBUG_VERBOSE("Returning INT type for array length access");
        return ast_create_primitive_type(table->arena, TYPE_INT);
    }

    /* array.push(elem) -> void */
    if (strcmp(name, "push") == 0)
    {
        Type *element_type = object_type->as.array.element_type;
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[1] = {element_type};
        DEBUG_VERBOSE("Returning function type for array push method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }

    /* array.pop() -> element_type */
    if (strcmp(name, "pop") == 0)
    {
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for array pop method");
        return ast_create_function_type(table->arena, object_type->as.array.element_type, param_types, 0);
    }

    /* array.clear() -> void */
    if (strcmp(name, "clear") == 0)
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for array clear method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }

    /* array.concat(other_array) -> array */
    if (strcmp(name, "concat") == 0)
    {
        Type *element_type = object_type->as.array.element_type;
        Type *param_array_type = ast_create_array_type(table->arena, element_type);
        Type *param_types[1] = {param_array_type};
        DEBUG_VERBOSE("Returning function type for array concat method");
        return ast_create_function_type(table->arena, object_type, param_types, 1);
    }

    /* array.indexOf(elem) -> int */
    if (strcmp(name, "indexOf") == 0)
    {
        Type *element_type = object_type->as.array.element_type;
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {element_type};
        DEBUG_VERBOSE("Returning function type for array indexOf method");
        return ast_create_function_type(table->arena, int_type, param_types, 1);
    }

    /* array.contains(elem) -> bool */
    if (strcmp(name, "contains") == 0)
    {
        Type *element_type = object_type->as.array.element_type;
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[1] = {element_type};
        DEBUG_VERBOSE("Returning function type for array contains method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* array.clone() -> array */
    if (strcmp(name, "clone") == 0)
    {
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for array clone method");
        return ast_create_function_type(table->arena, object_type, param_types, 0);
    }

    /* array.join(separator) -> str */
    if (strcmp(name, "join") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for array join method");
        return ast_create_function_type(table->arena, string_type, param_types, 1);
    }

    /* array.reverse() -> void */
    if (strcmp(name, "reverse") == 0)
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for array reverse method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }

    /* array.insert(elem, index) -> void */
    if (strcmp(name, "insert") == 0)
    {
        Type *element_type = object_type->as.array.element_type;
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[2] = {element_type, int_type};
        DEBUG_VERBOSE("Returning function type for array insert method");
        return ast_create_function_type(table->arena, void_type, param_types, 2);
    }

    /* array.remove(index) -> element_type */
    if (strcmp(name, "remove") == 0)
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *element_type = object_type->as.array.element_type;
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for array remove method");
        return ast_create_function_type(table->arena, element_type, param_types, 1);
    }

    /* Byte array extension methods - only available on byte[] */
    if (object_type->as.array.element_type->kind == TYPE_BYTE)
    {
        /* byte[].toString() -> str */
        if (strcmp(name, "toString") == 0)
        {
            Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
            Type *param_types[] = {NULL};
            DEBUG_VERBOSE("Returning function type for byte array toString method");
            return ast_create_function_type(table->arena, string_type, param_types, 0);
        }

        /* byte[].toStringLatin1() -> str */
        if (strcmp(name, "toStringLatin1") == 0)
        {
            Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
            Type *param_types[] = {NULL};
            DEBUG_VERBOSE("Returning function type for byte array toStringLatin1 method");
            return ast_create_function_type(table->arena, string_type, param_types, 0);
        }

        /* byte[].toHex() -> str */
        if (strcmp(name, "toHex") == 0)
        {
            Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
            Type *param_types[] = {NULL};
            DEBUG_VERBOSE("Returning function type for byte array toHex method");
            return ast_create_function_type(table->arena, string_type, param_types, 0);
        }

        /* byte[].toBase64() -> str */
        if (strcmp(name, "toBase64") == 0)
        {
            Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
            Type *param_types[] = {NULL};
            DEBUG_VERBOSE("Returning function type for byte array toBase64 method");
            return ast_create_function_type(table->arena, string_type, param_types, 0);
        }
    }

    /* Not an array method */
    return NULL;
}
