/*
 * code_gen_stmt_func_promote.c - Return value promotion helpers
 *
 * Handles promotion of return values from local arena to caller arena.
 */

#include "code_gen/code_gen_stmt.h"
#include "code_gen/code_gen_stmt_func_promote.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Generate promotion code for array return values */
void code_gen_promote_array_return(CodeGen *gen, Type *return_type, int indent)
{
    Type *elem_type = return_type->as.array.element_type;

    if (elem_type && elem_type->kind == TYPE_STRING)
    {
        /* String arrays need deep promotion */
        indented_fprintf(gen, indent, "_return_value = rt_managed_promote_array_string(__caller_arena__, __local_arena__, _return_value);\n");
    }
    else if (elem_type && elem_type->kind == TYPE_ARRAY)
    {
        /* 2D/3D arrays need deep promotion */
        Type *inner_elem = elem_type->as.array.element_type;
        if (inner_elem && inner_elem->kind == TYPE_STRING)
        {
            indented_fprintf(gen, indent, "_return_value = rt_managed_promote_array2_string(__caller_arena__, __local_arena__, _return_value);\n");
        }
        else if (inner_elem && inner_elem->kind == TYPE_ARRAY)
        {
            Type *innermost = inner_elem->as.array.element_type;
            if (innermost && innermost->kind == TYPE_STRING)
            {
                indented_fprintf(gen, indent, "_return_value = rt_managed_promote_array3_string(__caller_arena__, __local_arena__, _return_value);\n");
            }
            else
            {
                indented_fprintf(gen, indent, "_return_value = rt_managed_promote_array_handle_3d(__caller_arena__, __local_arena__, _return_value);\n");
            }
        }
        else
        {
            indented_fprintf(gen, indent, "_return_value = rt_managed_promote_array_handle(__caller_arena__, __local_arena__, _return_value);\n");
        }
    }
    else
    {
        /* Non-string, non-nested arrays */
        indented_fprintf(gen, indent, "_return_value = rt_managed_promote(__caller_arena__, __local_arena__, _return_value);\n");
    }
}

/* Generate promotion code for struct array fields with handle elements */
static void code_gen_promote_struct_array_field(CodeGen *gen, StructField *field,
                                                 const char *prefix, int indent)
{
    const char *c_field_name = field->c_alias != NULL ? field->c_alias : sn_mangle_name(gen->arena, field->name);
    Type *elem_type = field->type->as.array.element_type;

    if (elem_type && elem_type->kind == TYPE_STRING)
    {
        indented_fprintf(gen, indent, "%s.%s = rt_managed_promote_array_string(__caller_arena__, __local_arena__, %s.%s);\n",
                         prefix, c_field_name, prefix, c_field_name);
    }
    else if (elem_type && elem_type->kind == TYPE_ARRAY)
    {
        indented_fprintf(gen, indent, "%s.%s = rt_managed_promote_array_handle(__caller_arena__, __local_arena__, %s.%s);\n",
                         prefix, c_field_name, prefix, c_field_name);
    }
    else if (elem_type && elem_type->kind == TYPE_STRUCT && struct_has_handle_fields(elem_type))
    {
        /* Struct[] where struct has handle fields */
        const char *struct_c_name = elem_type->as.struct_type.c_alias != NULL
            ? elem_type->as.struct_type.c_alias
            : sn_mangle_name(gen->arena, elem_type->as.struct_type.name);

        indented_fprintf(gen, indent, "{ /* Promote handles in struct array elements */\n");
        indented_fprintf(gen, indent, "    %s *__parr__ = ((%s *)rt_managed_pin_array(__local_arena__, %s.%s));\n",
                         struct_c_name, struct_c_name, prefix, c_field_name);
        indented_fprintf(gen, indent, "    long __plen__ = rt_array_length(__parr__);\n");
        indented_fprintf(gen, indent, "    for (long __pi__ = 0; __pi__ < __plen__; __pi__++) {\n");

        /* Promote each handle field in struct elements */
        int struct_field_count = elem_type->as.struct_type.field_count;
        for (int fi = 0; fi < struct_field_count; fi++)
        {
            StructField *sf = &elem_type->as.struct_type.fields[fi];
            if (sf->type == NULL) continue;
            const char *sf_c_name = sf->c_alias != NULL ? sf->c_alias : sn_mangle_name(gen->arena, sf->name);

            if (sf->type->kind == TYPE_STRING)
            {
                indented_fprintf(gen, indent, "        __parr__[__pi__].%s = rt_managed_promote(__caller_arena__, __local_arena__, __parr__[__pi__].%s);\n",
                                 sf_c_name, sf_c_name);
            }
            else if (sf->type->kind == TYPE_ARRAY)
            {
                Type *sf_elem = sf->type->as.array.element_type;
                if (sf_elem && sf_elem->kind == TYPE_STRING)
                {
                    indented_fprintf(gen, indent, "        __parr__[__pi__].%s = rt_managed_promote_array_string(__caller_arena__, __local_arena__, __parr__[__pi__].%s);\n",
                                     sf_c_name, sf_c_name);
                }
                else
                {
                    indented_fprintf(gen, indent, "        __parr__[__pi__].%s = rt_managed_promote(__caller_arena__, __local_arena__, __parr__[__pi__].%s);\n",
                                     sf_c_name, sf_c_name);
                }
            }
        }

        indented_fprintf(gen, indent, "    }\n");
        indented_fprintf(gen, indent, "    %s.%s = rt_managed_promote(__caller_arena__, __local_arena__, %s.%s);\n",
                         prefix, c_field_name, prefix, c_field_name);
        indented_fprintf(gen, indent, "}\n");
    }
    else
    {
        indented_fprintf(gen, indent, "%s.%s = rt_managed_promote(__caller_arena__, __local_arena__, %s.%s);\n",
                         prefix, c_field_name, prefix, c_field_name);
    }
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
            indented_fprintf(gen, indent, "_return_value.%s = rt_managed_promote(__caller_arena__, __local_arena__, _return_value.%s);\n",
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
        indented_fprintf(gen, indent, "_return_value = rt_managed_promote(__caller_arena__, __local_arena__, _return_value);\n");
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
        indented_fprintf(gen, indent, "  _return_value = (__Closure__ *)rt_arena_alloc(__caller_arena__, __src_cl__->size);\n");
        indented_fprintf(gen, indent, "  memcpy(_return_value, __src_cl__, __src_cl__->size);\n");
        indented_fprintf(gen, indent, "  _return_value->arena = __caller_arena__; }\n");
    }
    else if (kind == TYPE_ANY)
    {
        indented_fprintf(gen, indent, "_return_value = rt_any_promote(__caller_arena__, _return_value);\n");
    }
}
