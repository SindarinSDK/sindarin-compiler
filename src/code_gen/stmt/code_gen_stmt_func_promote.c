/*
 * code_gen_stmt_func_promote.c - Return value promotion helpers
 *
 * Handles promotion of return values from local arena to caller arena.
 * Uses V2 arena functions - handles carry their own arena reference.
 */

#include "code_gen/stmt/code_gen_stmt.h"
#include "code_gen/stmt/code_gen_stmt_func_promote.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Generate promotion code for array return values.
 * Callbacks handle deep promotion automatically - just use rt_arena_v2_promote for all arrays. */
void code_gen_promote_array_return(CodeGen *gen, Type *return_type, int indent)
{
    (void)return_type;
    indented_fprintf(gen, indent, "_return_value = rt_arena_v2_promote(__caller_arena__, _return_value);\n");
}

/* Generate promotion code for struct array fields.
 * Callbacks handle deep promotion automatically - just use rt_arena_v2_promote. */
static void code_gen_promote_struct_array_field(CodeGen *gen, StructField *field,
                                                 const char *prefix, int indent)
{
    const char *c_field_name = field->c_alias != NULL ? field->c_alias : sn_mangle_name(gen->arena, field->name);
    indented_fprintf(gen, indent, "%s.%s = rt_arena_v2_promote(__caller_arena__, %s.%s);\n",
                     prefix, c_field_name, prefix, c_field_name);
}

/* Generate promotion code for struct return values */
void code_gen_promote_struct_return(CodeGen *gen, Type *return_type, int indent)
{
    int field_count = return_type->as.struct_type.field_count;

    for (int i = 0; i < field_count; i++)
    {
        StructField *field = &return_type->as.struct_type.fields[i];
        if (field->type == NULL) continue;

        const char *c_field_name = field->c_alias != NULL ? field->c_alias : sn_mangle_name(gen->arena, field->name);

        if (field->type->kind == TYPE_STRING)
        {
            indented_fprintf(gen, indent, "_return_value.%s = rt_arena_v2_promote(__caller_arena__, _return_value.%s);\n",
                             c_field_name, c_field_name);
        }
        else if (field->type->kind == TYPE_ARRAY)
        {
            code_gen_promote_struct_array_field(gen, field, "_return_value", indent);
        }
    }
}

/* Generate all return value promotion code for a function */
void code_gen_return_promotion(CodeGen *gen, Type *return_type, bool is_main, bool is_shared, int indent)
{
    if (is_main || is_shared || return_type == NULL)
        return;

    TypeKind kind = return_type->kind;

    if (kind == TYPE_STRING)
    {
        indented_fprintf(gen, indent, "_return_value = rt_arena_v2_promote(__caller_arena__, _return_value);\n");
    }
    else if (kind == TYPE_ARRAY)
    {
        code_gen_promote_array_return(gen, return_type, indent);
    }
    else if (kind == TYPE_STRUCT)
    {
        code_gen_promote_struct_return(gen, return_type, indent);
    }
    else if (kind == TYPE_FUNCTION)
    {
        /* Closures - copy to caller's arena */
        indented_fprintf(gen, indent, "{ __Closure__ *__src_cl__ = _return_value;\n");
        indented_fprintf(gen, indent, "  _return_value = (__Closure__ *)rt_arena_v2_alloc(__caller_arena__, __src_cl__->size);\n");
        indented_fprintf(gen, indent, "  memcpy(_return_value, __src_cl__, __src_cl__->size);\n");
        indented_fprintf(gen, indent, "  _return_value->arena = __caller_arena__; }\n");
    }
    else if (kind == TYPE_ANY)
    {
        indented_fprintf(gen, indent, "_return_value = rt_any_promote_v2(__caller_arena__, _return_value);\n");
    }
}
