/**
 * code_gen_expr_call_array.c - Code generation for array method calls
 *
 * Handles push, clear, pop and other array methods.
 */

#include "code_gen/code_gen_expr_call.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
#include "symbol_table.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Helper to get the arena to use for array mutations.
 * Mutations (push/pop/insert/remove/reverse) must allocate in the arena that
 * owns the array handle. For globals, that's __main_arena__; for locals/params,
 * it's the function's arena. */
static const char *get_arena_for_mutation(CodeGen *gen, Expr *object)
{
    if (object->type == EXPR_VARIABLE) {
        Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, object->as.variable.name);
        if (sym && is_handle_type(sym->type)) {
            if (sym->kind == SYMBOL_GLOBAL) {
                /* Global variables must be mutated using __main_arena__ so that
                 * reallocated handles persist across function calls. */
                return "__main_arena__";
            }
        }
    }
    return ARENA_VAR(gen);
}

/* Generate code for array.push(element) method */
static char *code_gen_array_push(CodeGen *gen, Expr *object, Type *element_type,
                                  Expr *arg)
{
    char *handle_str = NULL;
    char *lvalue_str = NULL;

    /* For global handle-type variables in a local arena context, we must NOT
     * use rt_managed_clone because:
     * 1. Clone creates a handle in the local arena
     * 2. But push_h expects a handle in the mutation arena (main arena for globals)
     * 3. The returned handle must be assigned back to the global variable
     * So we use the raw global variable directly for both reading and writing. */
    if (object->type == EXPR_VARIABLE && gen->current_arena_var != NULL) {
        Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, object->as.variable.name);
        if (sym && sym->kind == SYMBOL_GLOBAL && is_handle_type(sym->type)) {
            char *var_name = get_var_name(gen->arena, object->as.variable.name);
            char *mangled = sn_mangle_name(gen->arena, var_name);
            handle_str = mangled;
            lvalue_str = mangled;
        }
    }

    /* For non-global variables, evaluate in handle mode as before */
    if (handle_str == NULL) {
        bool prev_as_handle = gen->expr_as_handle;
        gen->expr_as_handle = true;
        handle_str = code_gen_expression(gen, object);
        gen->expr_as_handle = prev_as_handle;
        lvalue_str = handle_str;
    }

    /* For nested arrays in handle mode, generate arg in handle mode to get RtHandle */
    bool prev_arg_as_handle = gen->expr_as_handle;
    if (element_type->kind == TYPE_ARRAY && gen->current_arena_var != NULL) {
        gen->expr_as_handle = true;
    }
    char *arg_str = code_gen_expression(gen, arg);
    gen->expr_as_handle = prev_arg_as_handle;

    const char *arena_to_use = get_arena_for_mutation(gen, object);

    const char *push_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            push_func = "rt_array_push_long_h";
            break;
        case TYPE_INT32:
            push_func = "rt_array_push_int32_h";
            break;
        case TYPE_UINT:
            push_func = "rt_array_push_uint_h";
            break;
        case TYPE_UINT32:
            push_func = "rt_array_push_uint32_h";
            break;
        case TYPE_FLOAT:
            push_func = "rt_array_push_float_h";
            break;
        case TYPE_DOUBLE:
            push_func = "rt_array_push_double_h";
            break;
        case TYPE_CHAR:
            push_func = "rt_array_push_char_h";
            break;
        case TYPE_STRING:
            push_func = "rt_array_push_string_h";
            break;
        case TYPE_BOOL:
            push_func = "rt_array_push_bool_h";
            break;
        case TYPE_BYTE:
            push_func = "rt_array_push_byte_h";
            break;
        case TYPE_FUNCTION:
            push_func = "rt_array_push_voidptr_h";
            break;
        case TYPE_ARRAY:
            push_func = "rt_array_push_ptr_h";
            break;
        case TYPE_ANY:
            push_func = "rt_array_push_any_h";
            break;
        case TYPE_STRUCT:
            /* Struct types use a generic push with element size parameter.
             * The element is passed by pointer (address-of). */
            push_func = "rt_array_push_struct_h";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for push\n");
            exit(1);
    }

    /* push_h takes the RtHandle and returns the new handle.
     * Assign back to the lvalue so the handle stays valid after reallocation. */
    bool is_lvalue = (object->type == EXPR_VARIABLE ||
                      object->type == EXPR_MEMBER_ACCESS ||
                      object->type == EXPR_MEMBER);

    /* For struct types, use the struct push with element size.
     * The struct is passed by pointer (address-of). */
    if (element_type->kind == TYPE_STRUCT) {
        /* Get C type name for sizeof() */
        const char *c_type = get_c_type(gen->arena, element_type);
        if (is_lvalue) {
            return arena_sprintf(gen->arena, "(%s = %s(%s, %s, &%s, sizeof(%s)))",
                                 lvalue_str, push_func, arena_to_use, handle_str, arg_str, c_type);
        }
        return arena_sprintf(gen->arena, "%s(%s, %s, &%s, sizeof(%s))",
                             push_func, arena_to_use, handle_str, arg_str, c_type);
    }

    /* For pointer types (function/array), cast element to void*.
     * For nested arrays in handle mode, arg_str is RtHandle (uint32_t) so use uintptr_t. */
    if (element_type->kind == TYPE_FUNCTION || element_type->kind == TYPE_ARRAY) {
        const char *cast = (element_type->kind == TYPE_ARRAY && gen->current_arena_var != NULL)
            ? "(void *)(uintptr_t)" : "(void *)";
        if (is_lvalue) {
            return arena_sprintf(gen->arena, "(%s = %s(%s, %s, %s%s))",
                                 lvalue_str, push_func, arena_to_use, handle_str, cast, arg_str);
        }
        return arena_sprintf(gen->arena, "%s(%s, %s, %s%s)",
                             push_func, arena_to_use, handle_str, cast, arg_str);
    }

    if (is_lvalue) {
        return arena_sprintf(gen->arena, "(%s = %s(%s, %s, %s))",
                             lvalue_str, push_func, arena_to_use, handle_str, arg_str);
    }
    return arena_sprintf(gen->arena, "%s(%s, %s, %s)",
                         push_func, arena_to_use, handle_str, arg_str);
}

/* Generate code for array.clear() method */
static char *code_gen_array_clear(CodeGen *gen, Expr *object)
{
    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "rt_array_clear(%s)", object_str);
}

/* Generate code for array.pop() method */
static char *code_gen_array_pop(CodeGen *gen, Expr *object, Type *element_type)
{
    /* Evaluate object in handle mode to get the RtHandle */
    bool prev_as_handle = gen->expr_as_handle;
    gen->expr_as_handle = true;
    char *handle_str = code_gen_expression(gen, object);
    gen->expr_as_handle = prev_as_handle;

    const char *pop_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            pop_func = "rt_array_pop_long_h";
            break;
        case TYPE_INT32:
            pop_func = "rt_array_pop_int32_h";
            break;
        case TYPE_UINT:
            pop_func = "rt_array_pop_uint_h";
            break;
        case TYPE_UINT32:
            pop_func = "rt_array_pop_uint32_h";
            break;
        case TYPE_FLOAT:
            pop_func = "rt_array_pop_float_h";
            break;
        case TYPE_DOUBLE:
            pop_func = "rt_array_pop_double_h";
            break;
        case TYPE_CHAR:
            pop_func = "rt_array_pop_char_h";
            break;
        case TYPE_STRING:
            pop_func = "rt_array_pop_string_h";
            break;
        case TYPE_BOOL:
            pop_func = "rt_array_pop_bool_h";
            break;
        case TYPE_BYTE:
            pop_func = "rt_array_pop_byte_h";
            break;
        case TYPE_FUNCTION:
        case TYPE_ARRAY:
            pop_func = "rt_array_pop_ptr_h";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for pop\n");
            exit(1);
    }

    /* pop_h takes the RtHandle and the arena, returns the popped value */
    const char *arena_to_use = get_arena_for_mutation(gen, object);
    if (element_type->kind == TYPE_FUNCTION || element_type->kind == TYPE_ARRAY) {
        const char *elem_type_str = get_c_array_elem_type(gen->arena, element_type);
        return arena_sprintf(gen->arena, "(%s)%s(%s, %s)",
                             elem_type_str, pop_func, arena_to_use, handle_str);
    }
    return arena_sprintf(gen->arena, "%s(%s, %s)", pop_func, arena_to_use, handle_str);
}

/* Generate code for array.concat(other_array) method */
static char *code_gen_array_concat(CodeGen *gen, Expr *object, Type *element_type,
                                    Expr *arg, bool caller_wants_handle)
{
    /* Evaluate both arrays in raw pointer mode (pinned) for concat data */
    char *object_str = code_gen_expression(gen, object);
    char *arg_str = code_gen_expression(gen, arg);

    const char *concat_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            concat_func = "rt_array_concat_long_h";
            break;
        case TYPE_INT32:
            concat_func = "rt_array_concat_int32_h";
            break;
        case TYPE_UINT:
            concat_func = "rt_array_concat_uint_h";
            break;
        case TYPE_UINT32:
            concat_func = "rt_array_concat_uint32_h";
            break;
        case TYPE_FLOAT:
            concat_func = "rt_array_concat_float_h";
            break;
        case TYPE_DOUBLE:
            concat_func = "rt_array_concat_double_h";
            break;
        case TYPE_CHAR:
            concat_func = "rt_array_concat_char_h";
            break;
        case TYPE_STRING:
            concat_func = "rt_array_concat_string_h";
            break;
        case TYPE_BOOL:
            concat_func = "rt_array_concat_bool_h";
            break;
        case TYPE_BYTE:
            concat_func = "rt_array_concat_byte_h";
            break;
        case TYPE_FUNCTION:
        case TYPE_ARRAY:
            concat_func = "rt_array_concat_ptr_h";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for concat\n");
            exit(1);
    }

    /* concat_h takes two raw pointers and returns a new RtHandle.
     * If caller wants handle, return handle directly. Otherwise pin for raw pointer. */
    char *call_expr = arena_sprintf(gen->arena, "%s(%s, RT_HANDLE_NULL, %s, %s)",
                                    concat_func, ARENA_VAR(gen), object_str, arg_str);
    if (!caller_wants_handle && gen->current_arena_var != NULL)
    {
        const char *elem_c = get_c_array_elem_type(gen->arena, element_type);
        return arena_sprintf(gen->arena, "((%s *)rt_managed_pin_array(%s, %s))",
                             elem_c, ARENA_VAR(gen), call_expr);
    }
    return call_expr;
}

/* Generate code for array.indexOf(element) method */
static char *code_gen_array_indexof(CodeGen *gen, Expr *object, Type *element_type,
                                     Expr *arg)
{
    char *object_str = code_gen_expression(gen, object);
    char *arg_str = code_gen_expression(gen, arg);

    const char *indexof_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            indexof_func = "rt_array_indexOf_long";
            break;
        case TYPE_INT32:
            indexof_func = "rt_array_indexOf_int32";
            break;
        case TYPE_UINT:
            indexof_func = "rt_array_indexOf_uint";
            break;
        case TYPE_UINT32:
            indexof_func = "rt_array_indexOf_uint32";
            break;
        case TYPE_FLOAT:
            indexof_func = "rt_array_indexOf_float";
            break;
        case TYPE_DOUBLE:
            indexof_func = "rt_array_indexOf_double";
            break;
        case TYPE_CHAR:
            indexof_func = "rt_array_indexOf_char";
            break;
        case TYPE_STRING:
            if (gen->current_arena_var) {
                return arena_sprintf(gen->arena, "rt_array_indexOf_string_h(%s, %s, %s)",
                                     ARENA_VAR(gen), object_str, arg_str);
            }
            indexof_func = "rt_array_indexOf_string";
            break;
        case TYPE_BOOL:
            indexof_func = "rt_array_indexOf_bool";
            break;
        case TYPE_BYTE:
            indexof_func = "rt_array_indexOf_byte";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for indexOf\n");
            exit(1);
    }

    return arena_sprintf(gen->arena, "%s(%s, %s)", indexof_func, object_str, arg_str);
}

/* Generate code for array.contains(element) method */
static char *code_gen_array_contains(CodeGen *gen, Expr *object, Type *element_type,
                                      Expr *arg)
{
    char *object_str = code_gen_expression(gen, object);
    char *arg_str = code_gen_expression(gen, arg);

    const char *contains_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            contains_func = "rt_array_contains_long";
            break;
        case TYPE_INT32:
            contains_func = "rt_array_contains_int32";
            break;
        case TYPE_UINT:
            contains_func = "rt_array_contains_uint";
            break;
        case TYPE_UINT32:
            contains_func = "rt_array_contains_uint32";
            break;
        case TYPE_FLOAT:
            contains_func = "rt_array_contains_float";
            break;
        case TYPE_DOUBLE:
            contains_func = "rt_array_contains_double";
            break;
        case TYPE_CHAR:
            contains_func = "rt_array_contains_char";
            break;
        case TYPE_STRING:
            if (gen->current_arena_var) {
                return arena_sprintf(gen->arena, "rt_array_contains_string_h(%s, %s, %s)",
                                     ARENA_VAR(gen), object_str, arg_str);
            }
            contains_func = "rt_array_contains_string";
            break;
        case TYPE_BOOL:
            contains_func = "rt_array_contains_bool";
            break;
        case TYPE_BYTE:
            contains_func = "rt_array_contains_byte";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for contains\n");
            exit(1);
    }

    return arena_sprintf(gen->arena, "%s(%s, %s)", contains_func, object_str, arg_str);
}

/* Generate code for array.clone() method */
static char *code_gen_array_clone(CodeGen *gen, Expr *object, Type *element_type, bool handle_mode)
{
    char *object_str = code_gen_expression(gen, object);

    const char *suffix = code_gen_type_suffix(element_type);
    if (suffix == NULL) {
        fprintf(stderr, "Error: Unsupported array element type for clone\n");
        exit(1);
    }

    if (handle_mode && gen->current_arena_var != NULL) {
        return arena_sprintf(gen->arena, "rt_array_clone_%s_h(%s, RT_HANDLE_NULL, %s)",
                             suffix, ARENA_VAR(gen), object_str);
    }
    return arena_sprintf(gen->arena, "rt_array_clone_%s(%s, %s)", suffix, ARENA_VAR(gen), object_str);
}

/* Generate code for array.join(separator) method */
static char *code_gen_array_join(CodeGen *gen, Expr *object, Type *element_type,
                                  Expr *separator)
{
    char *object_str = code_gen_expression(gen, object);
    char *sep_str = code_gen_expression(gen, separator);

    const char *join_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            join_func = "rt_array_join_long";
            break;
        case TYPE_INT32:
            join_func = "rt_array_join_int32";
            break;
        case TYPE_UINT:
            join_func = "rt_array_join_uint";
            break;
        case TYPE_UINT32:
            join_func = "rt_array_join_uint32";
            break;
        case TYPE_FLOAT:
            join_func = "rt_array_join_float";
            break;
        case TYPE_DOUBLE:
            join_func = "rt_array_join_double";
            break;
        case TYPE_CHAR:
            join_func = "rt_array_join_char";
            break;
        case TYPE_STRING:
            join_func = gen->current_arena_var ? "rt_array_join_string_h" : "rt_array_join_string";
            break;
        case TYPE_BOOL:
            join_func = "rt_array_join_bool";
            break;
        case TYPE_BYTE:
            join_func = "rt_array_join_byte";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for join\n");
            exit(1);
    }

    return arena_sprintf(gen->arena, "%s(%s, %s, %s)", join_func, ARENA_VAR(gen), object_str, sep_str);
}

/* Generate code for array.reverse() method - in-place reverse */
static char *code_gen_array_reverse(CodeGen *gen, Expr *object, Type *element_type)
{
    char *object_str = code_gen_expression(gen, object);

    const char *rev_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            rev_func = "rt_array_rev_long";
            break;
        case TYPE_INT32:
            rev_func = "rt_array_rev_int32";
            break;
        case TYPE_UINT:
            rev_func = "rt_array_rev_uint";
            break;
        case TYPE_UINT32:
            rev_func = "rt_array_rev_uint32";
            break;
        case TYPE_FLOAT:
            rev_func = "rt_array_rev_float";
            break;
        case TYPE_DOUBLE:
            rev_func = "rt_array_rev_double";
            break;
        case TYPE_CHAR:
            rev_func = "rt_array_rev_char";
            break;
        case TYPE_STRING:
            rev_func = "rt_array_rev_string";
            break;
        case TYPE_BOOL:
            rev_func = "rt_array_rev_bool";
            break;
        case TYPE_BYTE:
            rev_func = "rt_array_rev_byte";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for reverse\n");
            exit(1);
    }

    /* reverse in-place: assign result back to variable */
    if (object->type == EXPR_VARIABLE) {
        /* In handle mode, use _h variant and assign to the handle variable */
        if (gen->current_arena_var != NULL && object->expr_type != NULL &&
            object->expr_type->kind == TYPE_ARRAY)
        {
            char *var_name = sn_mangle_name(gen->arena,
                get_var_name(gen->arena, object->as.variable.name));
            return arena_sprintf(gen->arena, "(%s = %s_h(%s, %s))",
                                 var_name, rev_func, ARENA_VAR(gen), object_str);
        }
        return arena_sprintf(gen->arena, "(%s = %s(%s, %s))", object_str, rev_func, ARENA_VAR(gen), object_str);
    }
    return arena_sprintf(gen->arena, "%s(%s, %s)", rev_func, ARENA_VAR(gen), object_str);
}

/* Generate code for array.insert(element, index) method - in-place insert */
static char *code_gen_array_insert(CodeGen *gen, Expr *object, Type *element_type,
                                    Expr *element, Expr *index)
{
    char *object_str = code_gen_expression(gen, object);
    char *elem_str = code_gen_expression(gen, element);
    char *idx_str = code_gen_expression(gen, index);

    const char *ins_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            ins_func = "rt_array_ins_long";
            break;
        case TYPE_INT32:
            ins_func = "rt_array_ins_int32";
            break;
        case TYPE_UINT:
            ins_func = "rt_array_ins_uint";
            break;
        case TYPE_UINT32:
            ins_func = "rt_array_ins_uint32";
            break;
        case TYPE_FLOAT:
            ins_func = "rt_array_ins_float";
            break;
        case TYPE_DOUBLE:
            ins_func = "rt_array_ins_double";
            break;
        case TYPE_CHAR:
            ins_func = "rt_array_ins_char";
            break;
        case TYPE_STRING:
            ins_func = "rt_array_ins_string";
            break;
        case TYPE_BOOL:
            ins_func = "rt_array_ins_bool";
            break;
        case TYPE_BYTE:
            ins_func = "rt_array_ins_byte";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for insert\n");
            exit(1);
    }

    /* insert in-place: assign result back to variable */
    if (object->type == EXPR_VARIABLE) {
        /* In handle mode, use _h variant and assign to the handle variable */
        if (gen->current_arena_var != NULL && object->expr_type != NULL &&
            object->expr_type->kind == TYPE_ARRAY)
        {
            char *var_name = sn_mangle_name(gen->arena,
                get_var_name(gen->arena, object->as.variable.name));
            return arena_sprintf(gen->arena, "(%s = %s_h(%s, %s, %s, %s))",
                                 var_name, ins_func, ARENA_VAR(gen), object_str, elem_str, idx_str);
        }
        return arena_sprintf(gen->arena, "(%s = %s(%s, %s, %s, %s))",
                             object_str, ins_func, ARENA_VAR(gen), object_str, elem_str, idx_str);
    }
    return arena_sprintf(gen->arena, "%s(%s, %s, %s, %s)",
                         ins_func, ARENA_VAR(gen), object_str, elem_str, idx_str);
}

/* Generate code for array.remove(index) method - in-place remove */
static char *code_gen_array_remove(CodeGen *gen, Expr *object, Type *element_type,
                                    Expr *index)
{
    char *object_str = code_gen_expression(gen, object);
    char *idx_str = code_gen_expression(gen, index);

    const char *rem_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            rem_func = "rt_array_rem_long";
            break;
        case TYPE_INT32:
            rem_func = "rt_array_rem_int32";
            break;
        case TYPE_UINT:
            rem_func = "rt_array_rem_uint";
            break;
        case TYPE_UINT32:
            rem_func = "rt_array_rem_uint32";
            break;
        case TYPE_FLOAT:
            rem_func = "rt_array_rem_float";
            break;
        case TYPE_DOUBLE:
            rem_func = "rt_array_rem_double";
            break;
        case TYPE_CHAR:
            rem_func = "rt_array_rem_char";
            break;
        case TYPE_STRING:
            rem_func = "rt_array_rem_string";
            break;
        case TYPE_BOOL:
            rem_func = "rt_array_rem_bool";
            break;
        case TYPE_BYTE:
            rem_func = "rt_array_rem_byte";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for remove\n");
            exit(1);
    }

    /* remove in-place: assign result back to variable */
    if (object->type == EXPR_VARIABLE) {
        /* In handle mode, use _h variant and assign to the handle variable */
        if (gen->current_arena_var != NULL && object->expr_type != NULL &&
            object->expr_type->kind == TYPE_ARRAY)
        {
            char *var_name = sn_mangle_name(gen->arena,
                get_var_name(gen->arena, object->as.variable.name));
            return arena_sprintf(gen->arena, "(%s = %s_h(%s, %s, %s))",
                                 var_name, rem_func, ARENA_VAR(gen), object_str, idx_str);
        }
        return arena_sprintf(gen->arena, "(%s = %s(%s, %s, %s))",
                             object_str, rem_func, ARENA_VAR(gen), object_str, idx_str);
    }
    return arena_sprintf(gen->arena, "%s(%s, %s, %s)",
                         rem_func, ARENA_VAR(gen), object_str, idx_str);
}

/* Generate code for byte[].toString() - UTF-8 decoding */
static char *code_gen_byte_array_to_string(CodeGen *gen, Expr *object)
{
    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "rt_byte_array_to_string(%s, %s)",
                         ARENA_VAR(gen), object_str);
}

/* Generate code for byte[].toStringLatin1() - Latin-1/ISO-8859-1 decoding */
static char *code_gen_byte_array_to_string_latin1(CodeGen *gen, Expr *object)
{
    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "rt_byte_array_to_string_latin1(%s, %s)",
                         ARENA_VAR(gen), object_str);
}

/* Generate code for byte[].toHex() - hexadecimal encoding */
static char *code_gen_byte_array_to_hex(CodeGen *gen, Expr *object)
{
    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "rt_byte_array_to_hex(%s, %s)",
                         ARENA_VAR(gen), object_str);
}

/* Generate code for byte[].toBase64() - Base64 encoding */
static char *code_gen_byte_array_to_base64(CodeGen *gen, Expr *object)
{
    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "rt_byte_array_to_base64(%s, %s)",
                         ARENA_VAR(gen), object_str);
}

/* Main dispatcher for array method calls */
char *code_gen_array_method_call(CodeGen *gen, Expr *expr, const char *method_name,
                                  Expr *object, Type *element_type, int arg_count,
                                  Expr **arguments)
{
    (void)expr; /* May be used for future enhancements */

    /* Most array methods need the object as a raw pointer (pinned form).
     * Force expr_as_handle=false so object evaluates to pinned pointer.
     * Methods that need the handle form (push, pop) manage their own state. */
    bool saved_handle_mode = gen->expr_as_handle;
    gen->expr_as_handle = false;

    char *result = NULL;

    /* Handle push(element) */
    if (strcmp(method_name, "push") == 0 && arg_count == 1) {
        result = code_gen_array_push(gen, object, element_type, arguments[0]);
    }
    /* Handle clear() */
    else if (strcmp(method_name, "clear") == 0 && arg_count == 0) {
        result = code_gen_array_clear(gen, object);
    }
    /* Handle pop() */
    else if (strcmp(method_name, "pop") == 0 && arg_count == 0) {
        result = code_gen_array_pop(gen, object, element_type);
    }
    /* Handle concat(other_array) */
    else if (strcmp(method_name, "concat") == 0 && arg_count == 1) {
        result = code_gen_array_concat(gen, object, element_type, arguments[0], saved_handle_mode);
    }
    /* Handle indexOf(element) */
    else if (strcmp(method_name, "indexOf") == 0 && arg_count == 1) {
        result = code_gen_array_indexof(gen, object, element_type, arguments[0]);
    }
    /* Handle contains(element) */
    else if (strcmp(method_name, "contains") == 0 && arg_count == 1) {
        result = code_gen_array_contains(gen, object, element_type, arguments[0]);
    }
    /* Handle clone() */
    else if (strcmp(method_name, "clone") == 0 && arg_count == 0) {
        result = code_gen_array_clone(gen, object, element_type, saved_handle_mode);
    }
    /* Handle join(separator) */
    else if (strcmp(method_name, "join") == 0 && arg_count == 1) {
        result = code_gen_array_join(gen, object, element_type, arguments[0]);
    }
    /* Handle reverse() */
    else if (strcmp(method_name, "reverse") == 0 && arg_count == 0) {
        result = code_gen_array_reverse(gen, object, element_type);
    }
    /* Handle insert(element, index) */
    else if (strcmp(method_name, "insert") == 0 && arg_count == 2) {
        result = code_gen_array_insert(gen, object, element_type, arguments[0], arguments[1]);
    }
    /* Handle remove(index) */
    else if (strcmp(method_name, "remove") == 0 && arg_count == 1) {
        result = code_gen_array_remove(gen, object, element_type, arguments[0]);
    }
    /* Byte array extension methods - only for byte[] */
    else if (element_type->kind == TYPE_BYTE) {
        if (strcmp(method_name, "toString") == 0 && arg_count == 0) {
            result = code_gen_byte_array_to_string(gen, object);
        }
        else if (strcmp(method_name, "toStringLatin1") == 0 && arg_count == 0) {
            result = code_gen_byte_array_to_string_latin1(gen, object);
        }
        else if (strcmp(method_name, "toHex") == 0 && arg_count == 0) {
            result = code_gen_byte_array_to_hex(gen, object);
        }
        else if (strcmp(method_name, "toBase64") == 0 && arg_count == 0) {
            result = code_gen_byte_array_to_base64(gen, object);
        }
    }

    gen->expr_as_handle = saved_handle_mode;

    /* If handle mode was active and the method returns a string (raw char *),
     * wrap the result to produce an RtHandle. Methods like toHex, toBase64,
     * toString return raw char* from C runtime functions. */
    if (saved_handle_mode && result != NULL && gen->current_arena_var != NULL)
    {
        /* Check if this is a string-returning method (byte encoding, join, etc.) */
        if ((element_type && element_type->kind == TYPE_BYTE &&
             (strcmp(method_name, "toHex") == 0 ||
              strcmp(method_name, "toBase64") == 0 ||
              strcmp(method_name, "toString") == 0 ||
              strcmp(method_name, "toStringLatin1") == 0)) ||
            strcmp(method_name, "join") == 0)
        {
            result = arena_sprintf(gen->arena, "rt_managed_strdup(%s, RT_HANDLE_NULL, %s)",
                                   ARENA_VAR(gen), result);
        }
    }

    return result;
}
