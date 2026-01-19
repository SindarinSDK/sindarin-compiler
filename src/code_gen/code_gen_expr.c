#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_expr_core.h"
#include "code_gen/code_gen_expr_binary.h"
#include "code_gen/code_gen_expr_lambda.h"
#include "code_gen/code_gen_expr_call.h"
#include "code_gen/code_gen_expr_thread.h"
#include "code_gen/code_gen_expr_array.h"
#include "code_gen/code_gen_expr_static.h"
#include "code_gen/code_gen_expr_string.h"
#include "code_gen/code_gen_util.h"
#include "code_gen/code_gen_stmt.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* Forward declarations */
char *code_gen_range_expression(CodeGen *gen, Expr *expr);

char *code_gen_increment_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_increment_expression");
    if (expr->as.operand->type != EXPR_VARIABLE)
    {
        exit(1);
    }
    char *var_name = get_var_name(gen->arena, expr->as.operand->as.variable.name);
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->as.operand->as.variable.name);

    /* For sync variables, use atomic increment */
    if (symbol && symbol->sync_mod == SYNC_ATOMIC)
    {
        return arena_sprintf(gen->arena, "__atomic_fetch_add(&%s, 1, __ATOMIC_SEQ_CST)", var_name);
    }

    /* For char/byte types, use inline increment to avoid type mismatch
     * (rt_post_inc_long reads 8 bytes, but char/byte are 1 byte) */
    if (symbol && symbol->type &&
        (symbol->type->kind == TYPE_CHAR || symbol->type->kind == TYPE_BYTE))
    {
        if (symbol->mem_qual == MEM_AS_REF)
        {
            return arena_sprintf(gen->arena, "(*%s)++", var_name);
        }
        return arena_sprintf(gen->arena, "%s++", var_name);
    }

    // For 'as ref' variables, they're already pointers, so pass directly
    if (symbol && symbol->mem_qual == MEM_AS_REF)
    {
        return arena_sprintf(gen->arena, "rt_post_inc_long(%s)", var_name);
    }
    return arena_sprintf(gen->arena, "rt_post_inc_long(&%s)", var_name);
}

char *code_gen_decrement_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_decrement_expression");
    if (expr->as.operand->type != EXPR_VARIABLE)
    {
        exit(1);
    }
    char *var_name = get_var_name(gen->arena, expr->as.operand->as.variable.name);
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->as.operand->as.variable.name);

    /* For sync variables, use atomic decrement */
    if (symbol && symbol->sync_mod == SYNC_ATOMIC)
    {
        return arena_sprintf(gen->arena, "__atomic_fetch_sub(&%s, 1, __ATOMIC_SEQ_CST)", var_name);
    }

    /* For char/byte types, use inline decrement to avoid type mismatch
     * (rt_post_dec_long reads 8 bytes, but char/byte are 1 byte) */
    if (symbol && symbol->type &&
        (symbol->type->kind == TYPE_CHAR || symbol->type->kind == TYPE_BYTE))
    {
        if (symbol->mem_qual == MEM_AS_REF)
        {
            return arena_sprintf(gen->arena, "(*%s)--", var_name);
        }
        return arena_sprintf(gen->arena, "%s--", var_name);
    }

    // For 'as ref' variables, they're already pointers, so pass directly
    if (symbol && symbol->mem_qual == MEM_AS_REF)
    {
        return arena_sprintf(gen->arena, "rt_post_dec_long(%s)", var_name);
    }
    return arena_sprintf(gen->arena, "rt_post_dec_long(&%s)", var_name);
}

char *code_gen_member_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_member_expression");
    MemberExpr *member = &expr->as.member;
    char *member_name_str = get_var_name(gen->arena, member->member_name);
    Type *object_type = member->object->expr_type;

    /* Handle namespace member access (namespace.symbol).
     * If the object has no type (expr_type is NULL) and is a variable,
     * this is a namespace member reference. Return just the function name
     * since C functions are referenced by name without namespace prefix. */
    if (object_type == NULL && member->object->type == EXPR_VARIABLE)
    {
        /* Look up the namespace symbol to check for c_alias */
        Token ns_name = member->object->as.variable.name;
        Symbol *func_sym = symbol_table_lookup_in_namespace(gen->symbol_table, ns_name, member->member_name);
        if (func_sym != NULL && func_sym->is_native && func_sym->c_alias != NULL)
        {
            return arena_strdup(gen->arena, func_sym->c_alias);
        }
        /* Namespace member access - emit just the function name */
        return member_name_str;
    }

    char *object_str = code_gen_expression(gen, member->object);

    // Handle array.length
    if (object_type->kind == TYPE_ARRAY && strcmp(member_name_str, "length") == 0) {
        return arena_sprintf(gen->arena, "rt_array_length(%s)", object_str);
    }

    // Handle string.length
    if (object_type->kind == TYPE_STRING && strcmp(member_name_str, "length") == 0) {
        return arena_sprintf(gen->arena, "rt_str_length(%s)", object_str);
    }

    /* Handle struct field access - generates object.field */
    if (object_type->kind == TYPE_STRUCT) {
        return arena_sprintf(gen->arena, "%s.%s", object_str, member_name_str);
    }

    /* Handle pointer-to-struct field access - generates object->field */
    if (object_type->kind == TYPE_POINTER &&
        object_type->as.pointer.base_type != NULL &&
        object_type->as.pointer.base_type->kind == TYPE_STRUCT) {
        return arena_sprintf(gen->arena, "%s->%s", object_str, member_name_str);
    }

    // Generic struct member access (not currently supported)
    fprintf(stderr, "Error: Unsupported member access on type\n");
    exit(1);
}

char *code_gen_range_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_range_expression");
    RangeExpr *range = &expr->as.range;

    char *start_str = code_gen_expression(gen, range->start);
    char *end_str = code_gen_expression(gen, range->end);

    return arena_sprintf(gen->arena, "rt_array_range(%s, %s, %s)", ARENA_VAR(gen), start_str, end_str);
}

char *code_gen_spread_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_spread_expression");
    // Spread expressions are typically handled in array literal context
    // but if used standalone, just return the array
    return code_gen_expression(gen, expr->as.spread.array);
}

static char *code_gen_sized_array_alloc_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_sized_array_alloc_expression");

    /* Extract fields from sized_array_alloc */
    SizedArrayAllocExpr *alloc = &expr->as.sized_array_alloc;
    Type *element_type = alloc->element_type;
    Expr *size_expr = alloc->size_expr;
    Expr *default_value = alloc->default_value;

    /* Determine the runtime function suffix based on element type */
    const char *suffix = NULL;
    switch (element_type->kind) {
        case TYPE_INT:
        case TYPE_LONG: suffix = "long"; break;
        case TYPE_INT32: suffix = "int32"; break;
        case TYPE_UINT: suffix = "uint"; break;
        case TYPE_UINT32: suffix = "uint32"; break;
        case TYPE_FLOAT: suffix = "float"; break;
        case TYPE_DOUBLE: suffix = "double"; break;
        case TYPE_CHAR: suffix = "char"; break;
        case TYPE_BOOL: suffix = "bool"; break;
        case TYPE_BYTE: suffix = "byte"; break;
        case TYPE_STRING: suffix = "string"; break;
        default:
            fprintf(stderr, "Error: Unsupported element type for sized array allocation\n");
            exit(1);
    }

    /* Generate code for the size expression */
    char *size_str = code_gen_expression(gen, size_expr);

    /* Generate code for the default value */
    char *default_str;
    if (default_value != NULL) {
        default_str = code_gen_expression(gen, default_value);
    } else {
        /* Use type-appropriate zero value when no default provided */
        switch (element_type->kind) {
            case TYPE_INT:
            case TYPE_LONG: default_str = "0"; break;
            case TYPE_INT32: default_str = "0"; break;
            case TYPE_UINT: default_str = "0"; break;
            case TYPE_UINT32: default_str = "0"; break;
            case TYPE_FLOAT: default_str = "0.0f"; break;
            case TYPE_DOUBLE: default_str = "0.0"; break;
            case TYPE_CHAR: default_str = "'\\0'"; break;
            case TYPE_BOOL: default_str = "0"; break;
            case TYPE_BYTE: default_str = "0"; break;
            case TYPE_STRING: default_str = "NULL"; break;
            default: default_str = "0"; break;
        }
    }

    /* Construct the runtime function call: rt_array_alloc_{suffix}(arena, size, default) */
    return arena_sprintf(gen->arena, "rt_array_alloc_%s(%s, %s, %s)",
                         suffix, ARENA_VAR(gen), size_str, default_str);
}

/**
 * Get the array clone function suffix for a given element type.
 * Returns NULL if no clone function is available for that type.
 */
static const char *get_array_clone_suffix(Type *element_type)
{
    if (element_type == NULL) return NULL;

    switch (element_type->kind)
    {
        case TYPE_INT:
        case TYPE_LONG: return "long";
        case TYPE_INT32: return "int32";
        case TYPE_UINT: return "uint";
        case TYPE_UINT32: return "uint32";
        case TYPE_DOUBLE: return "double";
        case TYPE_FLOAT: return "float";
        case TYPE_CHAR: return "char";
        case TYPE_BOOL: return "bool";
        case TYPE_BYTE: return "byte";
        case TYPE_STRING: return "string";
        default: return NULL;
    }
}

/**
 * Generate code for struct deep copy.
 * Creates a new struct with all array fields independently copied.
 */
static char *code_gen_struct_deep_copy(CodeGen *gen, Type *struct_type, char *operand_code)
{
    /* Generate a statement expression that:
     * 1. Copies the struct itself (shallow copy)
     * 2. For each array field, creates an independent copy
     */
    const char *struct_name = struct_type->as.struct_type.name;
    int field_count = struct_type->as.struct_type.field_count;

    /* Check if any fields need deep copying (arrays or strings) */
    bool has_array_fields = false;
    for (int i = 0; i < field_count; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];
        if (field->type != NULL &&
            (field->type->kind == TYPE_ARRAY || field->type->kind == TYPE_STRING))
        {
            has_array_fields = true;
            break;
        }
    }

    /* If no array fields, just do a simple struct copy */
    if (!has_array_fields)
    {
        return operand_code;
    }

    /* Generate statement expression for deep copy */
    char *result = arena_sprintf(gen->arena, "({\n        %s __deep_copy = %s;\n", struct_name, operand_code);

    /* For each array field, generate deep copy */
    for (int i = 0; i < field_count; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];
        if (field->type != NULL && field->type->kind == TYPE_ARRAY)
        {
            /* Get the element type and corresponding clone function */
            Type *element_type = field->type->as.array.element_type;
            const char *suffix = get_array_clone_suffix(element_type);
            if (suffix != NULL)
            {
                /* Copy array field using rt_array_clone_<type> */
                result = arena_sprintf(gen->arena, "%s        __deep_copy.%s = rt_array_clone_%s(%s, __deep_copy.%s);\n",
                                       result, field->name, suffix, ARENA_VAR(gen), field->name);
            }
            /* If no clone function available (e.g., nested arrays), leave as shallow copy */
        }
        else if (field->type != NULL && field->type->kind == TYPE_STRING)
        {
            /* Copy string field using rt_arena_strdup */
            result = arena_sprintf(gen->arena,
                                   "%s        __deep_copy.%s = __deep_copy.%s ? rt_arena_strdup(%s, __deep_copy.%s) : NULL;\n",
                                   result, field->name, field->name, ARENA_VAR(gen), field->name);
        }
    }

    /* Return the deep copied struct */
    result = arena_sprintf(gen->arena, "%s        __deep_copy;\n    })", result);

    return result;
}

/**
 * Generate code for 'as ref' expression - get pointer to value.
 * Counterpart to 'as val':
 * - For arrays: array variable is already a pointer in C, just return it
 * - For other values: use address-of operator &
 */
static char *code_gen_as_ref_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating as_ref expression");

    AsRefExpr *as_ref = &expr->as.as_ref;
    char *operand_code = code_gen_expression(gen, as_ref->operand);
    Type *operand_type = as_ref->operand->expr_type;

    if (operand_type != NULL && operand_type->kind == TYPE_ARRAY)
    {
        /* Arrays: the variable already holds a pointer to the array data.
         * In C, Sindarin arrays are represented as T* pointing to data. */
        return operand_code;
    }
    else
    {
        /* Other types: use address-of operator */
        return arena_sprintf(gen->arena, "(&(%s))", operand_code);
    }
}

/**
 * Generate code for 'as val' expression - pointer dereference/value extraction/struct deep copy.
 * For *int, *double, etc. - dereferences pointer to get value
 * For *char - converts null-terminated C string to Sn str
 * For structs - creates deep copy with independent array fields
 */
static char *code_gen_as_val_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating as_val expression");

    AsValExpr *as_val = &expr->as.as_val;
    char *operand_code = code_gen_expression(gen, as_val->operand);

    if (as_val->is_noop)
    {
        /* Operand is already an array type (e.g., from ptr[0..len] slice).
         * Just pass through without any transformation. */
        return operand_code;
    }
    else if (as_val->is_cstr_to_str)
    {
        /* *char => str: use rt_arena_strdup to copy null-terminated C string to arena.
         * Handle NULL pointer by returning empty string. */
        return arena_sprintf(gen->arena, "((%s) ? rt_arena_strdup(%s, %s) : rt_arena_strdup(%s, \"\"))",
                            operand_code, ARENA_VAR(gen), operand_code, ARENA_VAR(gen));
    }
    else if (as_val->is_struct_deep_copy)
    {
        /* Struct deep copy: copy struct and independently copy array fields */
        Type *operand_type = as_val->operand->expr_type;
        if (operand_type != NULL && operand_type->kind == TYPE_STRUCT)
        {
            return code_gen_struct_deep_copy(gen, operand_type, operand_code);
        }
        /* Fallback: should not happen, but just return operand */
        return operand_code;
    }
    else
    {
        /* Primitive pointer dereference: *int, *double, *float, etc. */
        return arena_sprintf(gen->arena, "(*(%s))", operand_code);
    }
}

/**
 * Get the runtime type tag constant for a type.
 */
static const char *get_type_tag_constant(TypeKind kind)
{
    switch (kind)
    {
        case TYPE_NIL: return "RT_ANY_NIL";
        case TYPE_INT: return "RT_ANY_INT";
        case TYPE_LONG: return "RT_ANY_LONG";
        case TYPE_INT32: return "RT_ANY_INT32";
        case TYPE_UINT: return "RT_ANY_UINT";
        case TYPE_UINT32: return "RT_ANY_UINT32";
        case TYPE_DOUBLE: return "RT_ANY_DOUBLE";
        case TYPE_FLOAT: return "RT_ANY_FLOAT";
        case TYPE_STRING: return "RT_ANY_STRING";
        case TYPE_CHAR: return "RT_ANY_CHAR";
        case TYPE_BOOL: return "RT_ANY_BOOL";
        case TYPE_BYTE: return "RT_ANY_BYTE";
        case TYPE_ARRAY: return "RT_ANY_ARRAY";
        case TYPE_FUNCTION: return "RT_ANY_FUNCTION";
        case TYPE_STRUCT: return "RT_ANY_STRUCT";
        case TYPE_ANY: return "RT_ANY_NIL";  /* any has no fixed tag */
        default: return "RT_ANY_NIL";
    }
}

/**
 * Generate code for sizeof expression.
 * sizeof(Type) - returns size in bytes of the type
 * sizeof(expr) - returns size in bytes of the expression's type
 */
static char *code_gen_sizeof_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating sizeof expression");

    SizeofExpr *sizeof_expr = &expr->as.sizeof_expr;

    if (sizeof_expr->type_operand != NULL)
    {
        /* sizeof(Type) - compile-time size of the type */
        const char *c_type = get_c_type(gen->arena, sizeof_expr->type_operand);
        return arena_sprintf(gen->arena, "(long long)sizeof(%s)", c_type);
    }
    else
    {
        /* sizeof(expr) - size of the expression's type */
        /* For expressions, we get the type from the type-checked expression */
        Type *expr_type = sizeof_expr->expr_operand->expr_type;
        const char *c_type = get_c_type(gen->arena, expr_type);
        return arena_sprintf(gen->arena, "(long long)sizeof(%s)", c_type);
    }
}

/**
 * Generate code for typeof expression.
 * typeof(value) - returns runtime type tag of any value
 * typeof(Type) - returns compile-time type tag constant
 */
static char *code_gen_typeof_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating typeof expression");

    TypeofExpr *typeof_expr = &expr->as.typeof_expr;

    if (typeof_expr->type_literal != NULL)
    {
        /* typeof(int), typeof(str), etc. - compile-time constant */
        return arena_sprintf(gen->arena, "%s", get_type_tag_constant(typeof_expr->type_literal->kind));
    }
    else
    {
        /* typeof(value) - get runtime type tag */
        char *operand_code = code_gen_expression(gen, typeof_expr->operand);
        Type *operand_type = typeof_expr->operand->expr_type;

        if (operand_type->kind == TYPE_ANY)
        {
            /* For any type, get the runtime tag */
            return arena_sprintf(gen->arena, "rt_any_get_tag(%s)", operand_code);
        }
        else
        {
            /* For concrete types, return compile-time constant */
            return arena_sprintf(gen->arena, "%s", get_type_tag_constant(operand_type->kind));
        }
    }
}

/**
 * Generate code for 'is' type check expression.
 * expr is Type - checks if any value is of the specified type
 */
static char *code_gen_is_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating is expression");

    IsExpr *is_expr = &expr->as.is_expr;
    char *operand_code = code_gen_expression(gen, is_expr->operand);
    const char *type_tag = get_type_tag_constant(is_expr->check_type->kind);

    /* For array types, also check the element type tag */
    if (is_expr->check_type->kind == TYPE_ARRAY &&
        is_expr->check_type->as.array.element_type != NULL)
    {
        Type *elem_type = is_expr->check_type->as.array.element_type;
        const char *elem_tag = get_type_tag_constant(elem_type->kind);
        return arena_sprintf(gen->arena, "((%s).tag == %s && (%s).element_tag == %s)",
                             operand_code, type_tag, operand_code, elem_tag);
    }

    /* For struct types, use the struct type ID for runtime type checking */
    if (is_expr->check_type->kind == TYPE_STRUCT)
    {
        int type_id = get_struct_type_id(is_expr->check_type);
        return arena_sprintf(gen->arena, "rt_any_is_struct_type(%s, %d)",
                             operand_code, type_id);
    }

    return arena_sprintf(gen->arena, "((%s).tag == %s)", operand_code, type_tag);
}

/* Helper to check if a type kind is numeric (for numeric type casts) */
static bool is_numeric_kind(TypeKind kind)
{
    return kind == TYPE_INT || kind == TYPE_INT32 ||
           kind == TYPE_UINT || kind == TYPE_UINT32 ||
           kind == TYPE_LONG || kind == TYPE_DOUBLE ||
           kind == TYPE_FLOAT || kind == TYPE_BYTE ||
           kind == TYPE_CHAR;
}

/**
 * Generate code for 'as Type' cast expression.
 * expr as Type - casts any value to concrete type (panics on mismatch),
 * or performs numeric type conversion (int -> byte, double -> int, etc.)
 */
static char *code_gen_as_type_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating as type expression");

    AsTypeExpr *as_type = &expr->as.as_type;
    char *operand_code = code_gen_expression(gen, as_type->operand);
    Type *target_type = as_type->target_type;
    Type *operand_type = as_type->operand->expr_type;

    /* Check if this is an any[] to T[] cast */
    if (operand_type != NULL &&
        operand_type->kind == TYPE_ARRAY &&
        operand_type->as.array.element_type != NULL &&
        operand_type->as.array.element_type->kind == TYPE_ANY &&
        target_type->kind == TYPE_ARRAY &&
        target_type->as.array.element_type != NULL)
    {
        Type *target_elem = target_type->as.array.element_type;
        const char *conv_func = NULL;
        switch (target_elem->kind)
        {
        case TYPE_INT:
        case TYPE_LONG:
            conv_func = "rt_array_from_any_long";
            break;
        case TYPE_INT32:
            conv_func = "rt_array_from_any_int32";
            break;
        case TYPE_UINT:
            conv_func = "rt_array_from_any_uint";
            break;
        case TYPE_UINT32:
            conv_func = "rt_array_from_any_uint32";
            break;
        case TYPE_DOUBLE:
            conv_func = "rt_array_from_any_double";
            break;
        case TYPE_FLOAT:
            conv_func = "rt_array_from_any_float";
            break;
        case TYPE_CHAR:
            conv_func = "rt_array_from_any_char";
            break;
        case TYPE_BOOL:
            conv_func = "rt_array_from_any_bool";
            break;
        case TYPE_BYTE:
            conv_func = "rt_array_from_any_byte";
            break;
        case TYPE_STRING:
            conv_func = "rt_array_from_any_string";
            break;
        default:
            break;
        }
        if (conv_func != NULL)
        {
            return arena_sprintf(gen->arena, "%s(%s, %s)", conv_func, ARENA_VAR(gen), operand_code);
        }
    }

    /* Check if this is a numeric type cast */
    if (operand_type != NULL && target_type != NULL &&
        (is_numeric_kind(operand_type->kind) || operand_type->kind == TYPE_BOOL) &&
        is_numeric_kind(target_type->kind))
    {
        /* Generate a C-style cast for numeric conversions */
        const char *c_type = get_c_type(gen->arena, target_type);
        return arena_sprintf(gen->arena, "((%s)(%s))", c_type, operand_code);
    }

    /* Use the unbox helper function for single any values */
    return code_gen_unbox_value(gen, operand_code, target_type);
}

/**
 * Generate code for struct literal expression.
 * Point { x: 1.0, y: 2.0 } -> (Point){ .x = 1.0, .y = 2.0 }
 */
static char *code_gen_struct_literal_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating struct literal expression");

    StructLiteralExpr *lit = &expr->as.struct_literal;
    Type *struct_type = lit->struct_type;

    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT)
    {
        fprintf(stderr, "Error: Struct literal has no resolved type\n");
        exit(1);
    }

    /* Use c_alias for C type name if available, otherwise use Sindarin name */
    const char *c_type_name = struct_type->as.struct_type.c_alias != NULL
        ? struct_type->as.struct_type.c_alias
        : struct_type->as.struct_type.name;
    int total_fields = struct_type->as.struct_type.field_count;

    /* Build the C compound literal: (StructName){ .field1 = val1, .field2 = val2, ... }
     * When inside an array compound literal, omit the outer (StructName) cast since
     * the array type already establishes the element type. This is required for TCC
     * compatibility which doesn't handle nested compound literal casts. */
    char *result;
    if (gen->in_array_compound_literal) {
        result = arena_strdup(gen->arena, "{ ");
    } else {
        result = arena_sprintf(gen->arena, "(%s){ ", c_type_name);
    }

    bool first = true;
    for (int i = 0; i < total_fields; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];

        /* Find if this field was explicitly initialized */
        Expr *init_value = NULL;
        for (int j = 0; j < lit->field_count; j++)
        {
            const char *init_name = lit->fields[j].name.start;
            int init_len = lit->fields[j].name.length;
            if ((int)strlen(field->name) == init_len &&
                strncmp(field->name, init_name, init_len) == 0)
            {
                init_value = lit->fields[j].value;
                break;
            }
        }

        /* Use default value if not explicitly initialized */
        if (init_value == NULL)
        {
            init_value = field->default_value;
        }

        /* Generate field initializer */
        if (init_value != NULL)
        {
            char *value_code = code_gen_expression(gen, init_value);
            if (!first)
            {
                result = arena_sprintf(gen->arena, "%s, ", result);
            }
            /* Use c_alias for field name if available, otherwise use Sindarin name */
            const char *c_field_name = field->c_alias != NULL ? field->c_alias : field->name;
            result = arena_sprintf(gen->arena, "%s.%s = %s", result, c_field_name, value_code);
            first = false;
        }
        else
        {
            /* No value provided and no default - use C default (zero) */
            /* For C compound literals, uninitialized fields are zeroed */
        }
    }

    result = arena_sprintf(gen->arena, "%s }", result);
    return result;
}

/**
 * Generate code for struct member access expression.
 * point.x -> point.x
 * ptr_to_struct.x -> ptr_to_struct->x (auto-dereference)
 */
static char *code_gen_member_access_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating member access expression");

    MemberAccessExpr *access = &expr->as.member_access;
    char *object_code = code_gen_expression(gen, access->object);
    Type *object_type = access->object->expr_type;

    /* Get field name - start with Sindarin name */
    char *field_name = arena_strndup(gen->arena, access->field_name.start, access->field_name.length);

    /* Look up field in struct type to get c_alias if available */
    Type *struct_type = object_type;
    if (struct_type != NULL && struct_type->kind == TYPE_POINTER)
    {
        struct_type = struct_type->as.pointer.base_type;
    }
    if (struct_type != NULL && struct_type->kind == TYPE_STRUCT)
    {
        StructField *field = ast_struct_get_field(struct_type, field_name);
        if (field != NULL && field->c_alias != NULL)
        {
            field_name = arena_strdup(gen->arena, field->c_alias);
        }
    }

    /* Check if this is pointer-to-struct (needs -> instead of .) */
    if (object_type != NULL && object_type->kind == TYPE_POINTER)
    {
        return arena_sprintf(gen->arena, "%s->%s", object_code, field_name);
    }

    return arena_sprintf(gen->arena, "%s.%s", object_code, field_name);
}

/**
 * Generate code for compound assignment expression.
 * x += 5 -> x = x + 5
 * arr[i] *= 2 -> arr.data[i] = arr.data[i] * 2
 * point.x -= 1 -> point.x = point.x - 1
 *
 * For sync variables, uses atomic operations where available:
 * sync_var += 5 -> __atomic_fetch_add(&sync_var, 5, __ATOMIC_SEQ_CST)
 * sync_var -= 5 -> __atomic_fetch_sub(&sync_var, 5, __ATOMIC_SEQ_CST)
 */
static char *code_gen_compound_assign_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating compound assign expression");

    CompoundAssignExpr *compound = &expr->as.compound_assign;
    Expr *target = compound->target;
    SnTokenType op = compound->operator;
    Type *target_type = target->expr_type;

    /* Check if target is a sync variable */
    Symbol *symbol = NULL;
    if (target->type == EXPR_VARIABLE)
    {
        symbol = symbol_table_lookup_symbol(gen->symbol_table, target->as.variable.name);
    }

    /* Generate code for the value */
    char *value_code = code_gen_expression(gen, compound->value);

    /* For sync variables, use atomic operations */
    if (symbol != NULL && symbol->sync_mod == SYNC_ATOMIC)
    {
        char *var_name = get_var_name(gen->arena, target->as.variable.name);
        const char *c_type = get_c_type(gen->arena, target_type);

        switch (op)
        {
            case TOKEN_PLUS:
                return arena_sprintf(gen->arena, "__atomic_fetch_add(&%s, %s, __ATOMIC_SEQ_CST)",
                                     var_name, value_code);
            case TOKEN_MINUS:
                return arena_sprintf(gen->arena, "__atomic_fetch_sub(&%s, %s, __ATOMIC_SEQ_CST)",
                                     var_name, value_code);
            case TOKEN_STAR:
            case TOKEN_SLASH:
            case TOKEN_MODULO:
            {
                /* Use CAS loop for *, /, % since no atomic builtin exists */
                const char *op_char = (op == TOKEN_STAR) ? "*" : (op == TOKEN_SLASH) ? "/" : "%";
                int cas_id = gen->temp_count++;
                return arena_sprintf(gen->arena,
                    "({ %s __old_%d__, __new_%d__; "
                    "do { __old_%d__ = __atomic_load_n(&%s, __ATOMIC_SEQ_CST); "
                    "__new_%d__ = __old_%d__ %s %s; } "
                    "while (!__atomic_compare_exchange_n(&%s, &__old_%d__, __new_%d__, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)); "
                    "__old_%d__; })",
                    c_type, cas_id, cas_id,
                    cas_id, var_name,
                    cas_id, cas_id, op_char, value_code,
                    var_name, cas_id, cas_id,
                    cas_id);
            }
            default:
                break;
        }
    }

    /* Get the operator symbol */
    const char *op_str;
    switch (op)
    {
        case TOKEN_PLUS: op_str = "+"; break;
        case TOKEN_MINUS: op_str = "-"; break;
        case TOKEN_STAR: op_str = "*"; break;
        case TOKEN_SLASH: op_str = "/"; break;
        case TOKEN_MODULO: op_str = "%"; break;
        default:
            fprintf(stderr, "Error: Unknown compound assignment operator\n");
            exit(1);
    }

    /* Generate code for the target */
    char *target_code = code_gen_expression(gen, target);

    /* For string concatenation (str += ...), use runtime function */
    if (target_type != NULL && target_type->kind == TYPE_STRING && op == TOKEN_PLUS)
    {
        /* Generate: target = rt_str_concat(arena, target, value) */
        return arena_sprintf(gen->arena, "%s = rt_str_concat(%s, %s, %s)",
                             target_code, ARENA_VAR(gen), target_code, value_code);
    }

    /* For numeric types, generate: target = target op value */
    return arena_sprintf(gen->arena, "%s = %s %s %s",
                         target_code, target_code, op_str, value_code);
}

/**
 * Generate code for struct member assignment expression.
 * point.x = 5.0 -> point.x = 5.0
 * ptr_to_struct.x = 5.0 -> ptr_to_struct->x = 5.0 (auto-dereference)
 */
static char *code_gen_member_assign_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating member assign expression");

    MemberAssignExpr *assign = &expr->as.member_assign;
    char *object_code = code_gen_expression(gen, assign->object);
    char *value_code = code_gen_expression(gen, assign->value);
    Type *object_type = assign->object->expr_type;

    /* Get field name - start with Sindarin name */
    char *field_name = arena_strndup(gen->arena, assign->field_name.start, assign->field_name.length);

    /* Look up field in struct type to get c_alias if available */
    Type *struct_type = object_type;
    if (struct_type != NULL && struct_type->kind == TYPE_POINTER)
    {
        struct_type = struct_type->as.pointer.base_type;
    }
    if (struct_type != NULL && struct_type->kind == TYPE_STRUCT)
    {
        StructField *field = ast_struct_get_field(struct_type, field_name);
        if (field != NULL && field->c_alias != NULL)
        {
            field_name = arena_strdup(gen->arena, field->c_alias);
        }
    }

    /* Check if this is pointer-to-struct (needs -> instead of .) */
    if (object_type != NULL && object_type->kind == TYPE_POINTER)
    {
        return arena_sprintf(gen->arena, "%s->%s = %s", object_code, field_name, value_code);
    }

    return arena_sprintf(gen->arena, "%s.%s = %s", object_code, field_name, value_code);
}

char *code_gen_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_expression");
    if (expr == NULL)
    {
        return arena_strdup(gen->arena, "0L");
    }
    switch (expr->type)
    {
    case EXPR_BINARY:
        return code_gen_binary_expression(gen, &expr->as.binary);
    case EXPR_UNARY:
        return code_gen_unary_expression(gen, &expr->as.unary);
    case EXPR_LITERAL:
        return code_gen_literal_expression(gen, &expr->as.literal);
    case EXPR_VARIABLE:
        return code_gen_variable_expression(gen, &expr->as.variable);
    case EXPR_ASSIGN:
        return code_gen_assign_expression(gen, &expr->as.assign);
    case EXPR_INDEX_ASSIGN:
        return code_gen_index_assign_expression(gen, &expr->as.index_assign);
    case EXPR_CALL:
        return code_gen_call_expression(gen, expr);
    case EXPR_ARRAY:
        return code_gen_array_expression(gen, expr);
    case EXPR_ARRAY_ACCESS:
        return code_gen_array_access_expression(gen, &expr->as.array_access);
    case EXPR_INCREMENT:
        return code_gen_increment_expression(gen, expr);
    case EXPR_DECREMENT:
        return code_gen_decrement_expression(gen, expr);
    case EXPR_INTERPOLATED:
        return code_gen_interpolated_expression(gen, &expr->as.interpol);
    case EXPR_MEMBER:
        return code_gen_member_expression(gen, expr);
    case EXPR_ARRAY_SLICE:
        return code_gen_array_slice_expression(gen, expr);
    case EXPR_RANGE:
        return code_gen_range_expression(gen, expr);
    case EXPR_SPREAD:
        return code_gen_spread_expression(gen, expr);
    case EXPR_LAMBDA:
        return code_gen_lambda_expression(gen, expr);
    case EXPR_STATIC_CALL:
        return code_gen_static_call_expression(gen, expr);
    case EXPR_SIZED_ARRAY_ALLOC:
        return code_gen_sized_array_alloc_expression(gen, expr);
    case EXPR_THREAD_SPAWN:
        return code_gen_thread_spawn_expression(gen, expr);
    case EXPR_THREAD_SYNC:
        return code_gen_thread_sync_expression(gen, expr);
    case EXPR_SYNC_LIST:
        /* Sync lists are only valid as part of thread sync [r1, r2]! */
        fprintf(stderr, "Error: Sync list without sync operator\n");
        exit(1);
    case EXPR_AS_VAL:
        return code_gen_as_val_expression(gen, expr);
    case EXPR_AS_REF:
        return code_gen_as_ref_expression(gen, expr);
    case EXPR_TYPEOF:
        return code_gen_typeof_expression(gen, expr);
    case EXPR_IS:
        return code_gen_is_expression(gen, expr);
    case EXPR_AS_TYPE:
        return code_gen_as_type_expression(gen, expr);
    case EXPR_STRUCT_LITERAL:
        return code_gen_struct_literal_expression(gen, expr);
    case EXPR_MEMBER_ACCESS:
        return code_gen_member_access_expression(gen, expr);
    case EXPR_MEMBER_ASSIGN:
        return code_gen_member_assign_expression(gen, expr);
    case EXPR_SIZEOF:
        return code_gen_sizeof_expression(gen, expr);
    case EXPR_COMPOUND_ASSIGN:
        return code_gen_compound_assign_expression(gen, expr);
    default:
        exit(1);
    }
    return NULL;
}
