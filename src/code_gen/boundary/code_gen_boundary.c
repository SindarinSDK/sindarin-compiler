/*
 * code_gen_boundary.c - Arena boundary protocol
 *
 * Every function with its own __local_arena__ is an ownership domain.
 * Handles entering must be cloned (callee owns copies).
 * Handles exiting must be promoted/cloned (caller owns result).
 *
 * Struct field promotion is delegated to generated __promote_*_inline__
 * callbacks (from code_gen_util_callbacks.c), which are the single
 * definition of how to promote a struct's handle fields. This avoids
 * hand-rolling field traversal at every boundary site.
 */

#include "code_gen/boundary/code_gen_boundary.h"
#include "code_gen/util/code_gen_util.h"
#include "code_gen/expr/code_gen_expr.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Boundary Enter — clone incoming handle params into __local_arena__
 * ============================================================================ */

void code_gen_boundary_enter(CodeGen *gen, Parameter *params, int param_count, int indent)
{
    for (int i = 0; i < param_count; i++)
    {
        Type *param_type = params[i].type;
        if (param_type == NULL) continue;

        if (param_type->kind == TYPE_STRING)
        {
            char *param_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, params[i].name));
            indented_fprintf(gen, indent, "%s = rt_arena_v2_clone(__local_arena__, %s);\n",
                             param_name, param_name);
            Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, params[i].name);
            if (sym) sym->kind = SYMBOL_LOCAL;
        }
        else if (param_type->kind == TYPE_STRUCT && params[i].mem_qualifier != MEM_AS_REF)
        {
            int field_count = param_type->as.struct_type.field_count;
            char *param_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, params[i].name));
            for (int f = 0; f < field_count; f++)
            {
                StructField *field = &param_type->as.struct_type.fields[f];
                if (field->type && field->type->kind == TYPE_STRING)
                {
                    const char *c_field_name = field->c_alias != NULL ? field->c_alias : sn_mangle_name(gen->arena, field->name);
                    indented_fprintf(gen, indent, "%s.%s = rt_arena_v2_clone(__local_arena__, %s.%s);\n",
                                     param_name, c_field_name, param_name, c_field_name);
                }
            }
        }
    }
}

/* ============================================================================
 * Boundary Exit — promote/clone outgoing return values and self fields
 * ============================================================================ */

/* Emit promote-or-clone for a single handle return value.
 * If the handle is on __local_arena__, promote (move) it.
 * If not (e.g. borrowed from self), clone (copy) it so the caller
 * receives a handle it exclusively owns. */
static void emit_handle_return_promote(CodeGen *gen, int indent, bool null_guard)
{
    if (null_guard)
    {
        indented_fprintf(gen, indent, "if (_return_value && _return_value->arena == __local_arena__)\n");
        indented_fprintf(gen, indent + 1, "_return_value = rt_arena_v2_promote(__caller_arena__, _return_value);\n");
        indented_fprintf(gen, indent, "else if (_return_value)\n");
        indented_fprintf(gen, indent + 1, "_return_value = rt_arena_v2_clone(__caller_arena__, _return_value);\n");
    }
    else
    {
        indented_fprintf(gen, indent, "if (_return_value && _return_value->arena == __local_arena__)\n");
        indented_fprintf(gen, indent + 1, "_return_value = rt_arena_v2_promote(__caller_arena__, _return_value);\n");
        indented_fprintf(gen, indent, "else if (_return_value)\n");
        indented_fprintf(gen, indent + 1, "_return_value = rt_arena_v2_clone(__caller_arena__, _return_value);\n");
    }
}

/* --- Struct return value promotion ---------------------------------------- */

/* Promote struct return value fields using generated __promote_*_inline__
 * callback, then reparent the struct's arena to the target arena. */
static void boundary_struct_return(CodeGen *gen, Type *return_type, const char *target_arena, int indent)
{
    Type *resolved = resolve_struct_type(gen, return_type);
    if (!resolved || !struct_has_handle_fields(resolved)) {
        /* No handle fields — just reparent the struct arena */
        if (resolved && !resolved->as.struct_type.is_packed && !resolved->as.struct_type.is_native)
        {
            indented_fprintf(gen, indent, "if (_return_value.__arena__)\n");
            indented_fprintf(gen, indent + 1, "rt_arena_v2_reparent(_return_value.__arena__, %s);\n", target_arena);
        }
        return;
    }

    code_gen_ensure_struct_callbacks(gen, return_type);

    const char *sn_name = resolved->as.struct_type.name;
    const char *guard_arena = gen->function_arena_var;

    indented_fprintf(gen, indent, "__promote_%s_inline__(%s, %s, &_return_value);\n",
                     sn_name, guard_arena, target_arena);

    /* Reparent the struct's arena to the target arena so it survives the
     * function's local arena destruction. */
    if (!resolved->as.struct_type.is_packed && !resolved->as.struct_type.is_native)
    {
        indented_fprintf(gen, indent, "if (_return_value.__arena__)\n");
        indented_fprintf(gen, indent + 1, "rt_arena_v2_reparent(_return_value.__arena__, %s);\n", target_arena);
    }
}

void code_gen_boundary_return(CodeGen *gen, Type *return_type, bool is_main,
                              const char *target_arena, int indent)
{
    if (is_main || return_type == NULL)
        return;

    TypeKind kind = return_type->kind;

    if (kind == TYPE_STRING || kind == TYPE_ARRAY)
    {
        emit_handle_return_promote(gen, indent, false);
    }
    else if (kind == TYPE_STRUCT)
    {
        Type *resolved_ret = resolve_struct_type(gen, return_type);
        if (resolved_ret->as.struct_type.is_native && resolved_ret->as.struct_type.c_alias != NULL)
        {
            /* Native struct returns are RtHandleV2* — promote or clone like other handles */
            emit_handle_return_promote(gen, indent, true);
        }
        else
        {
            boundary_struct_return(gen, return_type, target_arena, indent);
        }
    }
    else if (kind == TYPE_FUNCTION)
    {
        /* Closures are RtHandleV2* — promote or clone like other handles */
        emit_handle_return_promote(gen, indent, true);
    }
    else if (kind == TYPE_ANY)
    {
        indented_fprintf(gen, indent, "_return_value = rt_any_promote_v2(__caller_arena__, _return_value);\n");
    }
}

/* --- Self-field promotion -------------------------------------------------- */

void code_gen_boundary_self_promote(CodeGen *gen, StructDeclStmt *struct_decl, int indent)
{
    /* Look up the struct type to use the generated __promote_*_inline__ callback */
    Symbol *sym = symbol_table_lookup_type(gen->symbol_table, struct_decl->name);
    if (!sym || !sym->type) return;

    Type *self_type = resolve_struct_type(gen, sym->type);
    if (!self_type || !struct_has_handle_fields(self_type)) return;

    code_gen_ensure_struct_callbacks(gen, self_type);

    const char *sn_name = self_type->as.struct_type.name;
    indented_fprintf(gen, indent, "__promote_%s_inline__(__local_arena__, __sn__self->__arena__, __sn__self);\n", sn_name);
}
