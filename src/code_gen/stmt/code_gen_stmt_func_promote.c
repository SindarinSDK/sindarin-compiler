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
void code_gen_promote_array_return(CodeGen *gen, Type *return_type, const char *target_arena, int indent)
{
    (void)return_type;
    indented_fprintf(gen, indent, "_return_value = rt_arena_v2_promote(%s, _return_value);\n", target_arena);
}

/* Forward declaration for recursive promotion */
static void code_gen_promote_struct_fields(CodeGen *gen, Type *struct_type, const char *prefix, const char *target_arena, int indent);

/* Generate promotion code for struct return values */
void code_gen_promote_struct_return(CodeGen *gen, Type *return_type, const char *target_arena, int indent)
{
    code_gen_promote_struct_fields(gen, return_type, "_return_value", target_arena, indent);
}

/* Recursively generate promotion code for all handle fields in a struct.
 * Handles direct string/array fields and recurses into nested struct fields.
 * For struct methods, an arena guard prevents promoting handles already on
 * self->__arena__ (which would mark them dead if the method returns self). */
static void code_gen_promote_struct_fields(CodeGen *gen, Type *struct_type, const char *prefix, const char *target_arena, int indent)
{
    int field_count = struct_type->as.struct_type.field_count;
    /* Use function_arena_var as guard: only promote handles on the local arena
     * (about to be condemned). Handles on self->__arena__ are left alone. */
    const char *guard_arena = gen->function_arena_var;

    for (int i = 0; i < field_count; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];
        if (field->type == NULL) continue;

        const char *c_field_name = field->c_alias != NULL ? field->c_alias : sn_mangle_name(gen->arena, field->name);

        if (field->type->kind == TYPE_STRING)
        {
            indented_fprintf(gen, indent, "if (%s.%s && %s.%s->arena == %s)\n",
                             prefix, c_field_name, prefix, c_field_name, guard_arena);
            indented_fprintf(gen, indent + 1, "%s.%s = rt_arena_v2_promote(%s, %s.%s);\n",
                             prefix, c_field_name, target_arena, prefix, c_field_name);
        }
        else if (field->type->kind == TYPE_ARRAY)
        {
            indented_fprintf(gen, indent, "if (%s.%s && %s.%s->arena == %s)\n",
                             prefix, c_field_name, prefix, c_field_name, guard_arena);
            indented_fprintf(gen, indent + 1, "%s.%s = rt_arena_v2_promote(%s, %s.%s);\n",
                             prefix, c_field_name, target_arena, prefix, c_field_name);
        }
        else if (field->type->kind == TYPE_STRUCT && struct_has_handle_fields(field->type))
        {
            /* Recurse into nested struct to promote its handle fields */
            const char *nested_prefix = arena_sprintf(gen->arena, "%s.%s", prefix, c_field_name);
            code_gen_promote_struct_fields(gen, field->type, nested_prefix, target_arena, indent);
        }
    }
}

/* Forward declaration for recursive self-field promotion */
static void code_gen_promote_self_struct_fields(CodeGen *gen, Type *struct_type, const char *prefix, int indent);

/* Generate code to promote handle fields inside array elements from __local_arena__
 * to self->__arena__. This handles the case where array.push() memcpies a struct
 * whose handle fields are on __local_arena__, but the array itself wasn't reallocated
 * (so the array-level promote doesn't fire). */
static void code_gen_promote_self_array_elements(CodeGen *gen, const char *field_access,
                                                  Type *element_type, int indent)
{
    if (element_type->kind == TYPE_STRUCT && struct_has_handle_fields(element_type))
    {
        const char *c_struct_name = sn_mangle_name(gen->arena, element_type->as.struct_type.name);
        indented_fprintf(gen, indent, "if (%s) {\n", field_access);
        indented_fprintf(gen, indent + 1, "RtArrayMetadataV2 *__pm__ = (RtArrayMetadataV2 *)%s->ptr;\n", field_access);
        indented_fprintf(gen, indent + 1, "%s *__pa__ = (%s *)((char *)%s->ptr + sizeof(RtArrayMetadataV2));\n",
                         c_struct_name, c_struct_name, field_access);
        indented_fprintf(gen, indent + 1, "for (size_t __pi__ = 0; __pi__ < __pm__->size; __pi__++) {\n");

        /* Promote each handle field in the struct element */
        int fc = element_type->as.struct_type.field_count;
        for (int fi = 0; fi < fc; fi++)
        {
            StructField *sf = &element_type->as.struct_type.fields[fi];
            if (sf->type == NULL) continue;
            TypeKind fk = sf->type->kind;
            if (fk == TYPE_STRING || fk == TYPE_ARRAY || fk == TYPE_ANY || fk == TYPE_FUNCTION)
            {
                const char *cf = sf->c_alias != NULL ? sf->c_alias : sn_mangle_name(gen->arena, sf->name);
                indented_fprintf(gen, indent + 2, "if (__pa__[__pi__].%s && __pa__[__pi__].%s->arena == __local_arena__)\n",
                                 cf, cf);
                indented_fprintf(gen, indent + 3, "__pa__[__pi__].%s = rt_arena_v2_promote(__sn__self->__arena__, __pa__[__pi__].%s);\n",
                                 cf, cf);
            }
        }

        /* Clear the element's __arena__ — the struct is now inline in the array
         * and its handle fields have been promoted to self->__arena__. The original
         * per-element struct arena (child of __local_arena__) will be orphaned when
         * __local_arena__ is condemned. Setting it to NULL prevents later code from
         * condemning the freed arena. */
        indented_fprintf(gen, indent + 2, "if (__pa__[__pi__].__arena__ && __pa__[__pi__].__arena__ != __sn__self->__arena__)\n");
        indented_fprintf(gen, indent + 3, "__pa__[__pi__].__arena__ = NULL;\n");

        indented_fprintf(gen, indent + 1, "}\n");
        indented_fprintf(gen, indent, "}\n");
    }
    else if (element_type->kind == TYPE_STRING || element_type->kind == TYPE_ANY ||
             element_type->kind == TYPE_FUNCTION)
    {
        /* Arrays of string/any/function: elements are handle pointers */
        indented_fprintf(gen, indent, "if (%s) {\n", field_access);
        indented_fprintf(gen, indent + 1, "RtArrayMetadataV2 *__pm__ = (RtArrayMetadataV2 *)%s->ptr;\n", field_access);
        indented_fprintf(gen, indent + 1, "RtHandleV2 **__pa__ = (RtHandleV2 **)((char *)%s->ptr + sizeof(RtArrayMetadataV2));\n", field_access);
        indented_fprintf(gen, indent + 1, "for (size_t __pi__ = 0; __pi__ < __pm__->size; __pi__++) {\n");
        indented_fprintf(gen, indent + 2, "if (__pa__[__pi__] && __pa__[__pi__]->arena == __local_arena__)\n");
        indented_fprintf(gen, indent + 3, "__pa__[__pi__] = rt_arena_v2_promote(__sn__self->__arena__, __pa__[__pi__]);\n");
        indented_fprintf(gen, indent + 1, "}\n");
        indented_fprintf(gen, indent, "}\n");
    }
}

/* Promote self handle fields from __local_arena__ back to self->__arena__ before condemn.
 * Only promotes fields whose arena matches __local_arena__ (modified by THIS method call).
 * This prevents dangling pointers when the method's local arena is destroyed. */
void code_gen_promote_self_fields(CodeGen *gen, StructDeclStmt *struct_decl, int indent)
{
    for (int i = 0; i < struct_decl->field_count; i++)
    {
        StructField *field = &struct_decl->fields[i];
        if (field->type == NULL) continue;

        const char *c_field_name = field->c_alias != NULL ? field->c_alias : sn_mangle_name(gen->arena, field->name);
        const char *field_access = arena_sprintf(gen->arena, "__sn__self->%s", c_field_name);

        if (field->type->kind == TYPE_STRING)
        {
            indented_fprintf(gen, indent, "if (%s && %s->arena == __local_arena__)\n", field_access, field_access);
            indented_fprintf(gen, indent + 1, "%s = rt_arena_v2_promote(__sn__self->__arena__, %s);\n",
                             field_access, field_access);
        }
        else if (field->type->kind == TYPE_ARRAY)
        {
            /* Promote the array handle itself */
            indented_fprintf(gen, indent, "if (%s && %s->arena == __local_arena__)\n", field_access, field_access);
            indented_fprintf(gen, indent + 1, "%s = rt_arena_v2_promote(__sn__self->__arena__, %s);\n",
                             field_access, field_access);
            /* Also promote handle fields inside array elements — push may have
             * memcpy'd structs whose handle fields are on __local_arena__ */
            Type *elem_type = field->type->as.array.element_type;
            if (elem_type != NULL)
                code_gen_promote_self_array_elements(gen, field_access, elem_type, indent);
        }
        else if (field->type->kind == TYPE_STRUCT && struct_has_handle_fields(field->type))
        {
            code_gen_promote_self_struct_fields(gen, field->type, field_access, indent);
        }
        else if (field->type->kind == TYPE_FUNCTION)
        {
            indented_fprintf(gen, indent, "if (%s && %s->arena == __local_arena__) {\n", field_access, field_access);
            indented_fprintf(gen, indent + 1, "__Closure__ *__src_cl__ = %s;\n", field_access);
            indented_fprintf(gen, indent + 1, "%s = (__Closure__ *)rt_arena_v2_alloc(__sn__self->__arena__, __src_cl__->size);\n", field_access);
            indented_fprintf(gen, indent + 1, "memcpy(%s, __src_cl__, __src_cl__->size);\n", field_access);
            indented_fprintf(gen, indent + 1, "%s->arena = __sn__self->__arena__;\n", field_access);
            indented_fprintf(gen, indent, "}\n");
        }
        else if (field->type->kind == TYPE_ANY)
        {
            indented_fprintf(gen, indent, "%s = rt_any_promote_v2(__sn__self->__arena__, %s);\n",
                             field_access, field_access);
        }
    }
}

/* Recursively promote handle fields in nested structs on self.
 * Uses -> for top-level (self is pointer) and . for nested fields. */
static void code_gen_promote_self_struct_fields(CodeGen *gen, Type *struct_type, const char *prefix, int indent)
{
    int field_count = struct_type->as.struct_type.field_count;

    for (int i = 0; i < field_count; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];
        if (field->type == NULL) continue;

        const char *c_field_name = field->c_alias != NULL ? field->c_alias : sn_mangle_name(gen->arena, field->name);
        const char *field_access = arena_sprintf(gen->arena, "%s.%s", prefix, c_field_name);

        if (field->type->kind == TYPE_STRING || field->type->kind == TYPE_ARRAY)
        {
            indented_fprintf(gen, indent, "if (%s && %s->arena == __local_arena__)\n", field_access, field_access);
            indented_fprintf(gen, indent + 1, "%s = rt_arena_v2_promote(__sn__self->__arena__, %s);\n",
                             field_access, field_access);
        }
        else if (field->type->kind == TYPE_STRUCT && struct_has_handle_fields(field->type))
        {
            const char *nested_prefix = arena_sprintf(gen->arena, "%s.%s", prefix, c_field_name);
            code_gen_promote_self_struct_fields(gen, field->type, nested_prefix, indent);
        }
        else if (field->type->kind == TYPE_FUNCTION)
        {
            indented_fprintf(gen, indent, "if (%s && %s->arena == __local_arena__) {\n", field_access, field_access);
            indented_fprintf(gen, indent + 1, "__Closure__ *__src_cl__ = %s;\n", field_access);
            indented_fprintf(gen, indent + 1, "%s = (__Closure__ *)rt_arena_v2_alloc(__sn__self->__arena__, __src_cl__->size);\n", field_access);
            indented_fprintf(gen, indent + 1, "memcpy(%s, __src_cl__, __src_cl__->size);\n", field_access);
            indented_fprintf(gen, indent + 1, "%s->arena = __sn__self->__arena__;\n", field_access);
            indented_fprintf(gen, indent, "}\n");
        }
        else if (field->type->kind == TYPE_ANY)
        {
            indented_fprintf(gen, indent, "%s = rt_any_promote_v2(__sn__self->__arena__, %s);\n",
                             field_access, field_access);
        }
    }
}

/* Generate all return value promotion code for a function.
 * target_arena: "__sn__self->__arena__" for instance methods, "__caller_arena__" for regular functions */
void code_gen_return_promotion(CodeGen *gen, Type *return_type, bool is_main, bool is_shared, const char *target_arena, int indent)
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
        code_gen_promote_array_return(gen, return_type, "__caller_arena__", indent);
    }
    else if (kind == TYPE_STRUCT)
    {
        /* Struct returns use target_arena (self->__arena__ for instance methods)
         * to avoid killing handles shared between _return_value and self */
        code_gen_promote_struct_return(gen, return_type, target_arena, indent);
    }
    else if (kind == TYPE_FUNCTION)
    {
        /* Closures - copy to caller arena */
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
