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

/* Forward declaration for recursive promotion */
static void code_gen_promote_struct_fields(CodeGen *gen, Type *struct_type, const char *prefix, int indent);

/* Generate promotion code for struct return values */
void code_gen_promote_struct_return(CodeGen *gen, Type *return_type, int indent)
{
    code_gen_promote_struct_fields(gen, return_type, "_return_value", indent);
}

/* Recursively generate promotion code for all handle fields in a struct.
 * Handles direct string/array fields and recurses into nested struct fields. */
static void code_gen_promote_struct_fields(CodeGen *gen, Type *struct_type, const char *prefix, int indent)
{
    int field_count = struct_type->as.struct_type.field_count;

    for (int i = 0; i < field_count; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];
        if (field->type == NULL) continue;

        const char *c_field_name = field->c_alias != NULL ? field->c_alias : sn_mangle_name(gen->arena, field->name);

        if (field->type->kind == TYPE_STRING)
        {
            indented_fprintf(gen, indent, "%s.%s = rt_arena_v2_promote(__caller_arena__, %s.%s);\n",
                             prefix, c_field_name, prefix, c_field_name);
        }
        else if (field->type->kind == TYPE_ARRAY)
        {
            indented_fprintf(gen, indent, "%s.%s = rt_arena_v2_promote(__caller_arena__, %s.%s);\n",
                             prefix, c_field_name, prefix, c_field_name);
        }
        else if (field->type->kind == TYPE_STRUCT && struct_has_handle_fields(field->type))
        {
            /* Recurse into nested struct to promote its handle fields */
            const char *nested_prefix = arena_sprintf(gen->arena, "%s.%s", prefix, c_field_name);
            code_gen_promote_struct_fields(gen, field->type, nested_prefix, indent);
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
