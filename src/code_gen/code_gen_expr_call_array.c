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
 * Returns "NULL" for global variables to use malloc-based allocation,
 * otherwise returns the current arena. */
static const char *get_arena_for_mutation(CodeGen *gen, Expr *object)
{
    if (object->type == EXPR_VARIABLE) {
        Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, object->as.variable.name);
        if (sym != NULL && (sym->kind == SYMBOL_GLOBAL || sym->declaration_scope_depth <= 1)) {
            return "NULL";
        }
    }
    return ARENA_VAR(gen);
}

/* Generate code for array.push(element) method */
static char *code_gen_array_push(CodeGen *gen, Expr *object, Type *element_type,
                                  Expr *arg)
{
    char *object_str = code_gen_expression(gen, object);
    char *arg_str = code_gen_expression(gen, arg);
    const char *arena_to_use = get_arena_for_mutation(gen, object);

    const char *push_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            push_func = "rt_array_push_long";
            break;
        case TYPE_INT32:
            push_func = "rt_array_push_int32";
            break;
        case TYPE_UINT:
            push_func = "rt_array_push_uint";
            break;
        case TYPE_UINT32:
            push_func = "rt_array_push_uint32";
            break;
        case TYPE_FLOAT:
            push_func = "rt_array_push_float";
            break;
        case TYPE_DOUBLE:
            push_func = "rt_array_push_double";
            break;
        case TYPE_CHAR:
            push_func = "rt_array_push_char";
            break;
        case TYPE_STRING:
            push_func = "rt_array_push_string";
            break;
        case TYPE_BOOL:
            push_func = "rt_array_push_bool";
            break;
        case TYPE_BYTE:
            push_func = "rt_array_push_byte";
            break;
        case TYPE_FUNCTION:
        case TYPE_ARRAY:
            push_func = "rt_array_push_ptr";
            break;
        case TYPE_ANY:
            push_func = "rt_array_push_any";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for push\n");
            exit(1);
    }

    /* push returns new array pointer, assign back to variable if object is a variable */
    /* For pointer types (function/array), we need to cast to void** */
    if (element_type->kind == TYPE_FUNCTION || element_type->kind == TYPE_ARRAY) {
        if (object->type == EXPR_VARIABLE) {
            return arena_sprintf(gen->arena, "(%s = (void *)%s(%s, (void **)%s, (void *)%s))",
                                 object_str, push_func, arena_to_use, object_str, arg_str);
        }
        return arena_sprintf(gen->arena, "(void *)%s(%s, (void **)%s, (void *)%s)",
                             push_func, arena_to_use, object_str, arg_str);
    }

    if (object->type == EXPR_VARIABLE) {
        return arena_sprintf(gen->arena, "(%s = %s(%s, %s, %s))",
                             object_str, push_func, arena_to_use, object_str, arg_str);
    }
    return arena_sprintf(gen->arena, "%s(%s, %s, %s)",
                         push_func, arena_to_use, object_str, arg_str);
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
    char *object_str = code_gen_expression(gen, object);

    const char *pop_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            pop_func = "rt_array_pop_long";
            break;
        case TYPE_INT32:
            pop_func = "rt_array_pop_int32";
            break;
        case TYPE_UINT:
            pop_func = "rt_array_pop_uint";
            break;
        case TYPE_UINT32:
            pop_func = "rt_array_pop_uint32";
            break;
        case TYPE_FLOAT:
            pop_func = "rt_array_pop_float";
            break;
        case TYPE_DOUBLE:
            pop_func = "rt_array_pop_double";
            break;
        case TYPE_CHAR:
            pop_func = "rt_array_pop_char";
            break;
        case TYPE_STRING:
            pop_func = "rt_array_pop_string";
            break;
        case TYPE_BOOL:
            pop_func = "rt_array_pop_bool";
            break;
        case TYPE_BYTE:
            pop_func = "rt_array_pop_byte";
            break;
        case TYPE_FUNCTION:
        case TYPE_ARRAY:
            pop_func = "rt_array_pop_ptr";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for pop\n");
            exit(1);
    }

    /* For pointer types (function/array), we need to cast the result */
    if (element_type->kind == TYPE_FUNCTION || element_type->kind == TYPE_ARRAY) {
        const char *elem_type_str = get_c_type(gen->arena, element_type);
        return arena_sprintf(gen->arena, "(%s)%s((void **)%s)",
                             elem_type_str, pop_func, object_str);
    }
    return arena_sprintf(gen->arena, "%s(%s)", pop_func, object_str);
}

/* Generate code for array.concat(other_array) method */
static char *code_gen_array_concat(CodeGen *gen, Expr *object, Type *element_type,
                                    Expr *arg)
{
    char *object_str = code_gen_expression(gen, object);
    char *arg_str = code_gen_expression(gen, arg);

    const char *concat_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            concat_func = "rt_array_concat_long";
            break;
        case TYPE_INT32:
            concat_func = "rt_array_concat_int32";
            break;
        case TYPE_UINT:
            concat_func = "rt_array_concat_uint";
            break;
        case TYPE_UINT32:
            concat_func = "rt_array_concat_uint32";
            break;
        case TYPE_FLOAT:
            concat_func = "rt_array_concat_float";
            break;
        case TYPE_DOUBLE:
            concat_func = "rt_array_concat_double";
            break;
        case TYPE_CHAR:
            concat_func = "rt_array_concat_char";
            break;
        case TYPE_STRING:
            concat_func = "rt_array_concat_string";
            break;
        case TYPE_BOOL:
            concat_func = "rt_array_concat_bool";
            break;
        case TYPE_BYTE:
            concat_func = "rt_array_concat_byte";
            break;
        case TYPE_FUNCTION:
        case TYPE_ARRAY:
            concat_func = "rt_array_concat_ptr";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for concat\n");
            exit(1);
    }

    /* concat returns a new array, doesn't modify the original */
    /* For pointer types (function/array), we need to cast */
    if (element_type->kind == TYPE_FUNCTION || element_type->kind == TYPE_ARRAY) {
        const char *elem_type_str = get_c_type(gen->arena, element_type);
        return arena_sprintf(gen->arena, "(%s *)%s(%s, (void **)%s, (void **)%s)",
                             elem_type_str, concat_func, ARENA_VAR(gen), object_str, arg_str);
    }
    return arena_sprintf(gen->arena, "%s(%s, %s, %s)",
                         concat_func, ARENA_VAR(gen), object_str, arg_str);
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
static char *code_gen_array_clone(CodeGen *gen, Expr *object, Type *element_type)
{
    char *object_str = code_gen_expression(gen, object);

    const char *clone_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            clone_func = "rt_array_clone_long";
            break;
        case TYPE_INT32:
            clone_func = "rt_array_clone_int32";
            break;
        case TYPE_UINT:
            clone_func = "rt_array_clone_uint";
            break;
        case TYPE_UINT32:
            clone_func = "rt_array_clone_uint32";
            break;
        case TYPE_FLOAT:
            clone_func = "rt_array_clone_float";
            break;
        case TYPE_DOUBLE:
            clone_func = "rt_array_clone_double";
            break;
        case TYPE_CHAR:
            clone_func = "rt_array_clone_char";
            break;
        case TYPE_STRING:
            clone_func = "rt_array_clone_string";
            break;
        case TYPE_BOOL:
            clone_func = "rt_array_clone_bool";
            break;
        case TYPE_BYTE:
            clone_func = "rt_array_clone_byte";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for clone\n");
            exit(1);
    }

    return arena_sprintf(gen->arena, "%s(%s, %s)", clone_func, ARENA_VAR(gen), object_str);
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
            join_func = "rt_array_join_string";
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

    /* Handle push(element) */
    if (strcmp(method_name, "push") == 0 && arg_count == 1) {
        return code_gen_array_push(gen, object, element_type, arguments[0]);
    }

    /* Handle clear() */
    if (strcmp(method_name, "clear") == 0 && arg_count == 0) {
        return code_gen_array_clear(gen, object);
    }

    /* Handle pop() */
    if (strcmp(method_name, "pop") == 0 && arg_count == 0) {
        return code_gen_array_pop(gen, object, element_type);
    }

    /* Handle concat(other_array) */
    if (strcmp(method_name, "concat") == 0 && arg_count == 1) {
        return code_gen_array_concat(gen, object, element_type, arguments[0]);
    }

    /* Handle indexOf(element) */
    if (strcmp(method_name, "indexOf") == 0 && arg_count == 1) {
        return code_gen_array_indexof(gen, object, element_type, arguments[0]);
    }

    /* Handle contains(element) */
    if (strcmp(method_name, "contains") == 0 && arg_count == 1) {
        return code_gen_array_contains(gen, object, element_type, arguments[0]);
    }

    /* Handle clone() */
    if (strcmp(method_name, "clone") == 0 && arg_count == 0) {
        return code_gen_array_clone(gen, object, element_type);
    }

    /* Handle join(separator) */
    if (strcmp(method_name, "join") == 0 && arg_count == 1) {
        return code_gen_array_join(gen, object, element_type, arguments[0]);
    }

    /* Handle reverse() */
    if (strcmp(method_name, "reverse") == 0 && arg_count == 0) {
        return code_gen_array_reverse(gen, object, element_type);
    }

    /* Handle insert(element, index) */
    if (strcmp(method_name, "insert") == 0 && arg_count == 2) {
        return code_gen_array_insert(gen, object, element_type, arguments[0], arguments[1]);
    }

    /* Handle remove(index) */
    if (strcmp(method_name, "remove") == 0 && arg_count == 1) {
        return code_gen_array_remove(gen, object, element_type, arguments[0]);
    }

    /* Byte array extension methods - only for byte[] */
    if (element_type->kind == TYPE_BYTE) {
        /* Handle toString() - UTF-8 decoding */
        if (strcmp(method_name, "toString") == 0 && arg_count == 0) {
            return code_gen_byte_array_to_string(gen, object);
        }

        /* Handle toStringLatin1() - Latin-1/ISO-8859-1 decoding */
        if (strcmp(method_name, "toStringLatin1") == 0 && arg_count == 0) {
            return code_gen_byte_array_to_string_latin1(gen, object);
        }

        /* Handle toHex() - hexadecimal encoding */
        if (strcmp(method_name, "toHex") == 0 && arg_count == 0) {
            return code_gen_byte_array_to_hex(gen, object);
        }

        /* Handle toBase64() - Base64 encoding */
        if (strcmp(method_name, "toBase64") == 0 && arg_count == 0) {
            return code_gen_byte_array_to_base64(gen, object);
        }
    }

    /* Method not handled here - return NULL to let caller handle it */
    return NULL;
}
