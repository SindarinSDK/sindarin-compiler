#include "code_gen/code_gen_expr_core.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_expr_lambda.h"
#include "code_gen/code_gen_expr_array.h"
#include "code_gen/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

char *code_gen_literal_expression(CodeGen *gen, LiteralExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_literal_expression");
    Type *type = expr->type;
    switch (type->kind)
    {
    case TYPE_INT:
        return arena_sprintf(gen->arena, "%lldLL", (long long)expr->value.int_value);
    case TYPE_LONG:
        return arena_sprintf(gen->arena, "%lldLL", (long long)expr->value.int_value);
    case TYPE_DOUBLE:
    {
        char *str = arena_sprintf(gen->arena, "%.17g", expr->value.double_value);
        if (strchr(str, '.') == NULL && strchr(str, 'e') == NULL && strchr(str, 'E') == NULL)
        {
            str = arena_sprintf(gen->arena, "%s.0", str);
        }
        return str;
    }
    case TYPE_CHAR:
        return escape_char_literal(gen->arena, expr->value.char_value);
    case TYPE_STRING:
    {
        // This might break string interpolation
        /*char *escaped = escape_c_string(gen->arena, expr->value.string_value);
        return arena_sprintf(gen->arena, "rt_to_string_string(%s)", escaped);*/
        return escape_c_string(gen->arena, expr->value.string_value);
    }
    case TYPE_BOOL:
        return arena_sprintf(gen->arena, "%ldL", expr->value.bool_value ? 1L : 0L);
    case TYPE_BYTE:
        return arena_sprintf(gen->arena, "(uint8_t)%lldLL", (long long)expr->value.int_value);
    case TYPE_FLOAT:
    {
        char *str = arena_sprintf(gen->arena, "%.9gf", expr->value.double_value);
        if (strchr(str, '.') == NULL && strchr(str, 'e') == NULL && strchr(str, 'E') == NULL)
        {
            /* Remove the 'f' suffix and add ".0f" */
            str[strlen(str) - 1] = '\0';
            str = arena_sprintf(gen->arena, "%s.0f", str);
        }
        return str;
    }
    case TYPE_UINT:
        return arena_sprintf(gen->arena, "%lluULL", (unsigned long long)expr->value.int_value);
    case TYPE_UINT32:
        return arena_sprintf(gen->arena, "%uU", (unsigned int)expr->value.int_value);
    case TYPE_INT32:
        return arena_sprintf(gen->arena, "%d", (int)expr->value.int_value);
    case TYPE_NIL:
        return arena_strdup(gen->arena, "NULL");
    default:
        exit(1);
    }
    return NULL;
}

char *code_gen_variable_expression(CodeGen *gen, VariableExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_variable_expression");
    char *var_name = get_var_name(gen->arena, expr->name);

    /* Handle 'arena' built-in identifier - resolve to current arena variable */
    if (expr->name.length == 5 && strncmp(expr->name.start, "arena", 5) == 0)
    {
        if (gen->current_arena_var != NULL)
        {
            return arena_strdup(gen->arena, gen->current_arena_var);
        }
        /* Fallback to rt_current_arena() if no arena variable is available */
        return arena_strdup(gen->arena, "rt_current_arena()");
    }

    // Check if we're inside a lambda and this is a lambda parameter.
    // Lambda parameters shadow outer variables, so don't look up in symbol table.
    if (gen->enclosing_lambda_count > 0)
    {
        LambdaExpr *innermost = gen->enclosing_lambdas[gen->enclosing_lambda_count - 1];
        if (is_lambda_param(innermost, var_name))
        {
            // It's a parameter of the innermost lambda - use directly, no dereference
            return var_name;
        }
    }

    // Check if variable is 'as ref' - if so, dereference it
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->name);
    if (symbol && symbol->mem_qual == MEM_AS_REF)
    {
        return arena_sprintf(gen->arena, "(*%s)", var_name);
    }

    /* For native functions with c_alias, use the C name instead of Sindarin name */
    if (symbol && symbol->is_native && symbol->c_alias != NULL)
    {
        return arena_strdup(gen->arena, symbol->c_alias);
    }

    return var_name;
}

char *code_gen_assign_expression(CodeGen *gen, AssignExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_assign_expression");
    char *var_name = get_var_name(gen->arena, expr->name);
    char *value_str = code_gen_expression(gen, expr->value);
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->name);
    if (symbol == NULL)
    {
        exit(1);
    }
    Type *type = symbol->type;

    // Handle boxing when assigning to 'any' type
    if (type->kind == TYPE_ANY && expr->value->expr_type != NULL &&
        expr->value->expr_type->kind != TYPE_ANY)
    {
        value_str = code_gen_box_value(gen, value_str, expr->value->expr_type);
    }

    // Handle conversion when assigning typed array to any[], any[][], or any[][][]
    if (type->kind == TYPE_ARRAY &&
        type->as.array.element_type != NULL &&
        expr->value->expr_type != NULL &&
        expr->value->expr_type->kind == TYPE_ARRAY &&
        expr->value->expr_type->as.array.element_type != NULL)
    {
        Type *decl_elem = type->as.array.element_type;
        Type *src_elem = expr->value->expr_type->as.array.element_type;

        // Check for 3D array: any[][][] = T[][][]
        if (decl_elem->kind == TYPE_ARRAY &&
            decl_elem->as.array.element_type != NULL &&
            decl_elem->as.array.element_type->kind == TYPE_ARRAY &&
            decl_elem->as.array.element_type->as.array.element_type != NULL &&
            decl_elem->as.array.element_type->as.array.element_type->kind == TYPE_ANY &&
            src_elem->kind == TYPE_ARRAY &&
            src_elem->as.array.element_type != NULL &&
            src_elem->as.array.element_type->kind == TYPE_ARRAY &&
            src_elem->as.array.element_type->as.array.element_type != NULL &&
            src_elem->as.array.element_type->as.array.element_type->kind != TYPE_ANY)
        {
            Type *innermost_src = src_elem->as.array.element_type->as.array.element_type;
            const char *conv_func = NULL;
            switch (innermost_src->kind)
            {
            case TYPE_INT:
            case TYPE_INT32:
            case TYPE_UINT:
            case TYPE_UINT32:
            case TYPE_LONG:
                conv_func = "rt_array3_to_any_long";
                break;
            case TYPE_DOUBLE:
            case TYPE_FLOAT:
                conv_func = "rt_array3_to_any_double";
                break;
            case TYPE_CHAR:
                conv_func = "rt_array3_to_any_char";
                break;
            case TYPE_BOOL:
                conv_func = "rt_array3_to_any_bool";
                break;
            case TYPE_BYTE:
                conv_func = "rt_array3_to_any_byte";
                break;
            case TYPE_STRING:
                conv_func = "rt_array3_to_any_string";
                break;
            default:
                break;
            }
            if (conv_func != NULL)
            {
                value_str = arena_sprintf(gen->arena, "%s(%s, %s)", conv_func, ARENA_VAR(gen), value_str);
            }
        }
        // Check for 2D array: any[][] = T[][]
        else if (decl_elem->kind == TYPE_ARRAY &&
            decl_elem->as.array.element_type != NULL &&
            decl_elem->as.array.element_type->kind == TYPE_ANY &&
            src_elem->kind == TYPE_ARRAY &&
            src_elem->as.array.element_type != NULL &&
            src_elem->as.array.element_type->kind != TYPE_ANY)
        {
            Type *inner_src = src_elem->as.array.element_type;
            const char *conv_func = NULL;
            switch (inner_src->kind)
            {
            case TYPE_INT:
            case TYPE_INT32:
            case TYPE_UINT:
            case TYPE_UINT32:
            case TYPE_LONG:
                conv_func = "rt_array2_to_any_long";
                break;
            case TYPE_DOUBLE:
            case TYPE_FLOAT:
                conv_func = "rt_array2_to_any_double";
                break;
            case TYPE_CHAR:
                conv_func = "rt_array2_to_any_char";
                break;
            case TYPE_BOOL:
                conv_func = "rt_array2_to_any_bool";
                break;
            case TYPE_BYTE:
                conv_func = "rt_array2_to_any_byte";
                break;
            case TYPE_STRING:
                conv_func = "rt_array2_to_any_string";
                break;
            default:
                break;
            }
            if (conv_func != NULL)
            {
                value_str = arena_sprintf(gen->arena, "%s(%s, %s)", conv_func, ARENA_VAR(gen), value_str);
            }
        }
        // Check for 1D array: any[] = T[]
        else if (decl_elem->kind == TYPE_ANY && src_elem->kind != TYPE_ANY)
        {
            const char *conv_func = NULL;
            switch (src_elem->kind)
            {
            case TYPE_INT:
            case TYPE_INT32:
            case TYPE_UINT:
            case TYPE_UINT32:
            case TYPE_LONG:
                conv_func = "rt_array_to_any_long";
                break;
            case TYPE_DOUBLE:
            case TYPE_FLOAT:
                conv_func = "rt_array_to_any_double";
                break;
            case TYPE_CHAR:
                conv_func = "rt_array_to_any_char";
                break;
            case TYPE_BOOL:
                conv_func = "rt_array_to_any_bool";
                break;
            case TYPE_BYTE:
                conv_func = "rt_array_to_any_byte";
                break;
            case TYPE_STRING:
                conv_func = "rt_array_to_any_string";
                break;
            default:
                break;
            }
            if (conv_func != NULL)
            {
                value_str = arena_sprintf(gen->arena, "%s(%s, %s)", conv_func, ARENA_VAR(gen), value_str);
            }
        }
    }

    // Handle 'as ref' - dereference pointer for assignment
    if (symbol->mem_qual == MEM_AS_REF)
    {
        return arena_sprintf(gen->arena, "(*%s = %s)", var_name, value_str);
    }

    // Handle escaping struct assignments - copy to outer arena
    // When a struct value from an inner scope is assigned to an outer scope variable,
    // we need to ensure the struct data is allocated in the appropriate arena.
    if (type->kind == TYPE_STRUCT &&
        expr->value != NULL &&
        ast_expr_escapes_scope(expr->value) &&
        gen->current_arena_var != NULL)
    {
        // Get the struct type name for sizeof
        const char *struct_name = type->as.struct_type.name;
        if (struct_name != NULL)
        {
            // Generate: ({ StructType *_tmp = (StructType *)rt_arena_alloc(arena, sizeof(StructType));
            //             StructType __src_tmp__ = value;
            //             memcpy(_tmp, &__src_tmp__, sizeof(StructType));
            //             var = *_tmp; var; })
            // This allocates in the outer arena first, then copies using memcpy
            return arena_sprintf(gen->arena,
                "({ %s *__esc_tmp__ = (%s *)rt_arena_alloc(%s, sizeof(%s)); "
                "%s __esc_src__ = %s; "
                "memcpy(__esc_tmp__, &__esc_src__, sizeof(%s)); "
                "%s = *__esc_tmp__; %s; })",
                struct_name, struct_name, ARENA_VAR(gen), struct_name,
                struct_name, value_str,
                struct_name,
                var_name, var_name);
        }
    }

    if (type->kind == TYPE_STRING)
    {
        // Skip freeing old value in arena context - arena handles cleanup
        if (gen->current_arena_var != NULL)
        {
            return arena_sprintf(gen->arena, "(%s = %s)", var_name, value_str);
        }
        return arena_sprintf(gen->arena, "({ char *_val = %s; if (%s) rt_free_string(%s); %s = _val; _val; })",
                             value_str, var_name, var_name, var_name);
    }
    else
    {
        return arena_sprintf(gen->arena, "(%s = %s)", var_name, value_str);
    }
}

char *code_gen_index_assign_expression(CodeGen *gen, IndexAssignExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_index_assign_expression");
    char *array_str = code_gen_expression(gen, expr->array);
    char *index_str = code_gen_expression(gen, expr->index);
    char *value_str = code_gen_expression(gen, expr->value);

    // Check if index is provably non-negative (literal >= 0 or tracked loop counter)
    if (is_provably_non_negative(gen, expr->index))
    {
        // Non-negative index - direct array access
        return arena_sprintf(gen->arena, "(%s[%s] = %s)",
                             array_str, index_str, value_str);
    }

    // Check if index is a negative literal - can simplify to: arr[len + idx]
    if (expr->index->type == EXPR_LITERAL &&
        expr->index->as.literal.type != NULL &&
        (expr->index->as.literal.type->kind == TYPE_INT ||
         expr->index->as.literal.type->kind == TYPE_LONG))
    {
        // Negative literal - adjust by array length
        return arena_sprintf(gen->arena, "(%s[rt_array_length(%s) + %s] = %s)",
                             array_str, array_str, index_str, value_str);
    }

    // For potentially negative variable indices, generate runtime check
    return arena_sprintf(gen->arena, "(%s[(%s) < 0 ? rt_array_length(%s) + (%s) : (%s)] = %s)",
                         array_str, index_str, array_str, index_str, index_str, value_str);
}
