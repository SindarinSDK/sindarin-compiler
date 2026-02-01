#include "type_checker/type_checker_expr_cast.h"
#include "type_checker/type_checker_expr.h"
#include "type_checker/type_checker_util.h"
#include "debug.h"

/* 'as val' has multiple uses:
 * 1. Pointer dereference: *int -> int, *double -> double, etc.
 * 2. C string conversion: *char -> str (null-terminated string)
 * 3. Struct deep copy: Struct as val -> Struct (with array fields copied)
 * 4. Array pass-through: used with pointer slices (ptr[0..len] as val) */
Type *type_check_as_val(Expr *expr, SymbolTable *table)
{
    /* Enter as_val context so pointer slices know they're wrapped */
    as_val_context_enter();
    Type *operand_type = type_check_expr(expr->as.as_val.operand, table);
    as_val_context_exit();

    if (operand_type == NULL)
    {
        type_error(expr->token, "Invalid operand in 'as val' expression");
        return NULL;
    }
    else if (operand_type->kind == TYPE_ARRAY)
    {
        /* Array type: 'as val' is a no-op, returns same type.
         * This supports ptr[0..len] as val where the slice already
         * produces an array type. */
        expr->as.as_val.is_cstr_to_str = false;
        expr->as.as_val.is_noop = true;
        expr->as.as_val.is_struct_deep_copy = false;
        DEBUG_VERBOSE("'as val' on array type (no-op): returns same array type");
        return operand_type;
    }
    else if (operand_type->kind == TYPE_STRUCT)
    {
        /* Struct type: 'as val' creates a deep copy of the struct.
         * Array fields inside the struct are independently copied.
         * This ensures modifications to the copy don't affect the original. */
        expr->as.as_val.is_cstr_to_str = false;
        expr->as.as_val.is_noop = false;
        expr->as.as_val.is_struct_deep_copy = true;
        DEBUG_VERBOSE("'as val' on struct type: returns deep copy of struct");
        return operand_type;
    }
    else if (operand_type->kind != TYPE_POINTER)
    {
        type_error(expr->token, "'as val' requires a pointer, array, or struct type operand");
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
            expr->as.as_val.is_cstr_to_str = true;
            expr->as.as_val.is_noop = false;
            expr->as.as_val.is_struct_deep_copy = false;
            DEBUG_VERBOSE("'as val' converts *char to str (null-terminated string)");
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else
        {
            /* Result type is the base type of the pointer */
            expr->as.as_val.is_cstr_to_str = false;
            expr->as.as_val.is_noop = false;
            expr->as.as_val.is_struct_deep_copy = false;
            DEBUG_VERBOSE("'as val' unwraps pointer to type: %d", base_type->kind);
            return base_type;
        }
    }
}

/* 'as ref' gets a pointer to a value - counterpart to 'as val':
 * int as ref -> *int
 * byte[] as ref -> *byte (pointer to array data)
 * Only allowed in native function context */
Type *type_check_as_ref(Expr *expr, SymbolTable *table)
{
    Type *operand_type = type_check_expr(expr->as.as_ref.operand, table);
    if (operand_type == NULL)
    {
        type_error(expr->token, "Invalid operand in 'as ref' expression");
        return NULL;
    }
    else if (!native_context_is_active())
    {
        type_error(expr->token, "'as ref' is only allowed in native function bodies");
        return NULL;
    }
    else if (operand_type->kind == TYPE_ARRAY)
    {
        /* Array: return pointer to element type (e.g., byte[] -> *byte) */
        Type *elem_type = operand_type->as.array.element_type;
        DEBUG_VERBOSE("'as ref' on array: returns *element_type");
        return ast_create_pointer_type(table->arena, elem_type);
    }
    else if (operand_type->kind == TYPE_POINTER)
    {
        type_error(expr->token, "'as ref' cannot be applied to pointer type (already a pointer)");
        return NULL;
    }
    else
    {
        /* For primitives and other types: return pointer to that type */
        DEBUG_VERBOSE("'as ref' on value: returns pointer type");
        return ast_create_pointer_type(table->arena, operand_type);
    }
}

/* typeof operator returns a type tag value for runtime comparison */
/* typeof(value) - returns runtime type of any value */
/* typeof(Type) - returns the type tag for that type */
Type *type_check_typeof(Expr *expr, SymbolTable *table)
{
    if (expr->as.typeof_expr.type_literal != NULL)
    {
        /* typeof(int), typeof(str), etc. - always valid */
        DEBUG_VERBOSE("typeof type literal: returns type tag");
        return ast_create_primitive_type(table->arena, TYPE_INT);  /* Type tag is int */
    }
    else if (expr->as.typeof_expr.operand != NULL)
    {
        Type *operand_type = type_check_expr(expr->as.typeof_expr.operand, table);
        if (operand_type == NULL)
        {
            type_error(expr->token, "Invalid operand in typeof expression");
            return NULL;
        }
        else if (operand_type->kind != TYPE_ANY)
        {
            /* For non-any types, typeof is a compile-time constant */
            DEBUG_VERBOSE("typeof non-any value: compile-time type tag");
        }
        else
        {
            /* For any type, typeof is a runtime operation */
            DEBUG_VERBOSE("typeof any value: runtime type tag");
        }
        return ast_create_primitive_type(table->arena, TYPE_INT);  /* Type tag is int */
    }
    else
    {
        type_error(expr->token, "typeof requires an operand or type");
        return NULL;
    }
}

/* 'is' operator: checks if an any value is of a specific type */
/* Returns bool */
Type *type_check_is(Expr *expr, SymbolTable *table)
{
    Type *operand_type = type_check_expr(expr->as.is_expr.operand, table);
    if (operand_type == NULL)
    {
        type_error(expr->token, "Invalid operand in 'is' expression");
        return NULL;
    }
    else if (operand_type->kind != TYPE_ANY)
    {
        type_error(expr->token, "'is' operator requires an 'any' type operand");
        return NULL;
    }
    else
    {
        DEBUG_VERBOSE("'is' type check: returns bool");
        return ast_create_primitive_type(table->arena, TYPE_BOOL);
    }
}

/* 'as Type' operator: casts an any value to a concrete type,
 * or performs numeric type conversions (int -> byte, double -> int, etc.) */
/* Returns the target type */
Type *type_check_as_type(Expr *expr, SymbolTable *table)
{
    Type *operand_type = type_check_expr(expr->as.as_type.operand, table);
    Type *target_type = expr->as.as_type.target_type;

    if (operand_type == NULL)
    {
        type_error(expr->token, "Invalid operand in 'as' cast expression");
        return NULL;
    }
    else if (operand_type->kind == TYPE_ANY)
    {
        /* Single any value cast to concrete type */
        DEBUG_VERBOSE("'as' type cast: returns target type %d", target_type->kind);
        return target_type;
    }
    else if (operand_type->kind == TYPE_ARRAY &&
             operand_type->as.array.element_type != NULL &&
             operand_type->as.array.element_type->kind == TYPE_ANY)
    {
        /* any[] cast to T[] */
        if (target_type->kind != TYPE_ARRAY)
        {
            type_error(expr->token, "'as <type>' cast from any[] requires array target type");
            return NULL;
        }
        else
        {
            DEBUG_VERBOSE("'as' array type cast: returns target type %d", target_type->kind);
            return target_type;
        }
    }
    else if (is_numeric_type(operand_type) && is_numeric_type(target_type))
    {
        /* Numeric type conversion: int -> byte, double -> int, etc. */
        DEBUG_VERBOSE("'as' numeric type cast: %d -> %d", operand_type->kind, target_type->kind);
        return target_type;
    }
    else if (operand_type->kind == TYPE_BOOL && is_numeric_type(target_type))
    {
        /* Bool to numeric conversion: true -> 1, false -> 0 */
        DEBUG_VERBOSE("'as' bool to numeric cast: -> %d", target_type->kind);
        return target_type;
    }
    else
    {
        type_error(expr->token, "'as <type>' cast requires an 'any', 'any[]', or numeric type operand");
        return NULL;
    }
}
