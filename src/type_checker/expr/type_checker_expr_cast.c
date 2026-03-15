#include "type_checker/expr/type_checker_expr_cast.h"
#include "type_checker/expr/type_checker_expr.h"
#include "type_checker/util/type_checker_util.h"
#include "debug.h"

/* addressOf(expr) - get pointer to a value:
 * addressOf(int_val) -> *int
 * addressOf(byte_arr) -> *byte (pointer to array data)
 * Only allowed in native function context */
Type *type_check_address_of(Expr *expr, SymbolTable *table)
{
    Type *operand_type = type_check_expr(expr->as.address_of.operand, table);
    if (operand_type == NULL)
    {
        type_error(expr->token, "Invalid operand in addressOf() expression");
        return NULL;
    }
    else if (!native_context_is_active())
    {
        type_error(expr->token, "addressOf() is only allowed in native function bodies");
        return NULL;
    }
    else if (operand_type->kind == TYPE_ARRAY)
    {
        /* Array: return pointer to element type (e.g., byte[] -> *byte) */
        Type *elem_type = operand_type->as.array.element_type;
        DEBUG_VERBOSE("addressOf() on array: returns *element_type");
        return ast_create_pointer_type(table->arena, elem_type);
    }
    else if (operand_type->kind == TYPE_POINTER)
    {
        type_error(expr->token, "addressOf() cannot be applied to pointer type (already a pointer)");
        return NULL;
    }
    else
    {
        /* For primitives and other types: return pointer to that type */
        DEBUG_VERBOSE("addressOf() on value: returns pointer type");
        return ast_create_pointer_type(table->arena, operand_type);
    }
}

/* valueOf(expr) - dereference pointer or convert *char to str:
 * valueOf(*int) -> int
 * valueOf(*char) -> str (null-terminated string conversion)
 * valueOf(ptr[0..len]) -> array (pointer slice unwrap)
 * Only allowed in native function context */
Type *type_check_value_of(Expr *expr, SymbolTable *table)
{
    /* Enter value_of context so pointer slices know they're wrapped */
    value_of_context_enter();
    Type *operand_type = type_check_expr(expr->as.value_of.operand, table);
    value_of_context_exit();

    if (operand_type == NULL)
    {
        type_error(expr->token, "Invalid operand in valueOf() expression");
        return NULL;
    }
    else if (operand_type->kind == TYPE_ARRAY)
    {
        /* Array type: valueOf() is a no-op, returns same type.
         * This supports valueOf(ptr[0..len]) where the slice already
         * produces an array type. */
        expr->as.value_of.is_cstr_to_str = false;
        expr->as.value_of.is_noop = true;
        DEBUG_VERBOSE("valueOf() on array type (no-op): returns same array type");
        return operand_type;
    }
    else if (operand_type->kind != TYPE_POINTER)
    {
        type_error(expr->token, "valueOf() requires a pointer or array type operand");
        return NULL;
    }
    else if (operand_type->as.pointer.base_type != NULL &&
             operand_type->as.pointer.base_type->kind == TYPE_OPAQUE)
    {
        /* Opaque types cannot be dereferenced */
        type_error(expr->token, "Cannot dereference pointer to opaque type");
        return NULL;
    }
    else
    {
        Type *base_type = operand_type->as.pointer.base_type;
        /* Special case: *char => str (null-terminated string conversion) */
        if (base_type->kind == TYPE_CHAR)
        {
            expr->as.value_of.is_cstr_to_str = true;
            expr->as.value_of.is_noop = false;
            DEBUG_VERBOSE("valueOf() converts *char to str (null-terminated string)");
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else
        {
            /* Result type is the base type of the pointer */
            expr->as.value_of.is_cstr_to_str = false;
            expr->as.value_of.is_noop = false;
            DEBUG_VERBOSE("valueOf() unwraps pointer to type: %d", base_type->kind);
            return base_type;
        }
    }
}

/* copyOf(expr) - deep copy a value:
 * copyOf(my_struct) -> Struct (with array/string fields independently copied)
 * copyOf(my_array)  -> new array with copied elements
 * copyOf(my_string) -> new string (strdup)
 * NOT restricted to native context */
Type *type_check_copy_of(Expr *expr, SymbolTable *table)
{
    Type *operand_type = type_check_expr(expr->as.copy_of.operand, table);
    if (operand_type == NULL)
    {
        type_error(expr->token, "Invalid operand in copyOf() expression");
        return NULL;
    }
    else if (operand_type->kind == TYPE_STRUCT ||
             operand_type->kind == TYPE_ARRAY ||
             operand_type->kind == TYPE_STRING)
    {
        return operand_type;
    }
    else
    {
        type_error(expr->token, "copyOf() requires a struct, array, or string operand");
        return NULL;
    }
}

