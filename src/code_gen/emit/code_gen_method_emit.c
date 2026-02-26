/**
 * code_gen_method_emit.c - Struct method emission for code generation
 *
 * Contains functions for emitting struct method forward declarations
 * and implementations during code generation.
 */

#include "code_gen/emit/code_gen_method_emit.h"
#include "code_gen/util/code_gen_util.h"
#include "code_gen/stmt/code_gen_stmt.h"
#include "code_gen/stmt/code_gen_stmt_func_promote.h"
#include "arena.h"
#include <string.h>
#include <ctype.h>

int code_gen_emit_struct_method_forwards(CodeGen *gen, Stmt **statements, int count)
{
    int method_count = 0;
    for (int i = 0; i < count; i++)
    {
        Stmt *stmt = statements[i];
        if (stmt->type == STMT_STRUCT_DECL)
        {
            StructDeclStmt *struct_decl = &stmt->as.struct_decl;
            char *raw_struct_name = arena_strndup(gen->arena, struct_decl->name.start, struct_decl->name.length);
            char *struct_name = sn_mangle_name(gen->arena, raw_struct_name);

            /* Create lowercase struct name for native method naming (uses raw name) */
            char *struct_name_lower = arena_strdup(gen->arena, raw_struct_name);
            for (char *p = struct_name_lower; *p; p++)
            {
                *p = (char)tolower((unsigned char)*p);
            }

            for (int j = 0; j < struct_decl->method_count; j++)
            {
                StructMethod *method = &struct_decl->methods[j];

                if (method_count == 0)
                {
                    indented_fprintf(gen, 0, "/* Struct method forward declarations */\n");
                }

                const char *ret_type;
                if (method->is_native && method->body == NULL &&
                    method->return_type != NULL && method->return_type->kind == TYPE_STRING) {
                    /* Native methods returning str use RtHandleV2* (handle-based strings) */
                    ret_type = "RtHandleV2 *";
                } else if (method->is_native && method->body == NULL &&
                           method->return_type != NULL && method->return_type->kind == TYPE_ARRAY) {
                    /* Native methods returning arrays return RtHandleV2* directly */
                    ret_type = "RtHandleV2 *";
                } else if (method->is_native && method->body == NULL &&
                           method->return_type != NULL && method->return_type->kind == TYPE_STRUCT &&
                           method->return_type->as.struct_type.is_native && method->has_arena_param) {
                    /* Native methods returning native structs with arena use RtHandleV2* */
                    ret_type = "RtHandleV2 *";
                } else if (method->return_type != NULL && method->return_type->kind == TYPE_STRUCT &&
                           method->return_type->as.struct_type.is_native &&
                           method->return_type->as.struct_type.c_alias != NULL) {
                    /* Sindarin methods returning native struct types use RtHandleV2* for promotion */
                    ret_type = "RtHandleV2 *";
                } else {
                    ret_type = get_c_type(gen->arena, method->return_type);
                }

                if (method->is_native && method->body == NULL)
                {
                    /* Native method - generate extern declaration.
                     * If c_alias is set, use that as the function name.
                     * Otherwise, use the rt_{struct_lowercase}_{method_name} convention. */
                    const char *func_name;
                    if (method->c_alias != NULL)
                    {
                        func_name = method->c_alias;
                    }
                    else
                    {
                        func_name = arena_sprintf(gen->arena, "rt_%s_%s", struct_name_lower, method->name);
                    }

                    /* For opaque handle types (native struct with c_alias), use the C type directly */
                    const char *self_c_type;
                    if (struct_decl->is_native && struct_decl->c_alias != NULL)
                    {
                        /* Opaque handle: self type is already a pointer (e.g., RtDate *) */
                        self_c_type = arena_sprintf(gen->arena, "%s *", struct_decl->c_alias);
                    }
                    else if (struct_decl->pass_self_by_ref)
                    {
                        /* as ref: self is passed by pointer */
                        self_c_type = arena_sprintf(gen->arena, "%s *", struct_name);
                    }
                    else
                    {
                        self_c_type = struct_name;
                    }

                    if (method->is_static)
                    {
                        /* Static native: extern RetType func_name(params); */
                        indented_fprintf(gen, 0, "extern %s %s(",
                                         ret_type, func_name);
                        /* If method has implicit arena param, prepend RtArena* */
                        if (method->has_arena_param)
                        {
                            indented_fprintf(gen, 0, "RtArena *");
                            if (method->param_count > 0)
                            {
                                indented_fprintf(gen, 0, ", ");
                            }
                        }
                        if (method->param_count == 0 && !method->has_arena_param)
                        {
                            indented_fprintf(gen, 0, "void");
                        }
                        else
                        {
                            for (int k = 0; k < method->param_count; k++)
                            {
                                if (k > 0)
                                    indented_fprintf(gen, 0, ", ");
                                Parameter *param = &method->params[k];
                                const char *param_type = get_c_native_param_type(gen->arena, param->type);
                                indented_fprintf(gen, 0, "%s", param_type);
                            }
                        }
                        indented_fprintf(gen, 0, ");\n");
                    }
                    else
                    {
                        /* Instance native: extern RetType func_name(self_type self, params); */
                        /* If method has implicit arena param, prepend RtArena* */
                        if (method->has_arena_param)
                        {
                            indented_fprintf(gen, 0, "extern %s %s(RtArena *, %s",
                                             ret_type, func_name, self_c_type);
                        }
                        else
                        {
                            indented_fprintf(gen, 0, "extern %s %s(%s",
                                             ret_type, func_name, self_c_type);
                        }
                        for (int k = 0; k < method->param_count; k++)
                        {
                            Parameter *param = &method->params[k];
                            const char *param_type = get_c_native_param_type(gen->arena, param->type);
                            indented_fprintf(gen, 0, ", %s", param_type);
                        }
                        indented_fprintf(gen, 0, ");\n");
                    }
                }
                else
                {
                    /* Non-native method - regular forward declaration with arena */
                    if (method->is_static)
                    {
                        /* Static method: no self parameter */
                        if (method->param_count == 0)
                        {
                            indented_fprintf(gen, 0, "%s %s_%s(RtArenaV2 *__caller_arena__);\n",
                                             ret_type, struct_name, method->name);
                        }
                        else
                        {
                            indented_fprintf(gen, 0, "%s %s_%s(RtArenaV2 *__caller_arena__",
                                             ret_type, struct_name, method->name);
                            for (int k = 0; k < method->param_count; k++)
                            {
                                Parameter *param = &method->params[k];
                                const char *param_type = get_c_param_type(gen->arena, param->type);
                                char *param_name = sn_mangle_name(gen->arena, arena_strndup(gen->arena, param->name.start, param->name.length));
                                indented_fprintf(gen, 0, ", %s %s", param_type, param_name);
                            }
                            indented_fprintf(gen, 0, ");\n");
                        }
                    }
                    else
                    {
                        /* Instance method: first parameter is self (pointer to struct) */
                        /* For opaque handle types (native struct with c_alias), self is already a pointer */
                        if (struct_decl->is_native && struct_decl->c_alias != NULL)
                        {
                            /* Opaque handle: self type is the C alias pointer */
                            indented_fprintf(gen, 0, "%s %s_%s(RtArenaV2 *__caller_arena__, %s *__sn__self",
                                             ret_type, struct_name, method->name, struct_decl->c_alias);
                        }
                        else
                        {
                            /* Regular struct: self is pointer to struct */
                            indented_fprintf(gen, 0, "%s %s_%s(RtArenaV2 *__caller_arena__, %s *__sn__self",
                                             ret_type, struct_name, method->name, struct_name);
                        }
                        for (int k = 0; k < method->param_count; k++)
                        {
                            Parameter *param = &method->params[k];
                            const char *param_type = get_c_param_type(gen->arena, param->type);
                            char *param_name = sn_mangle_name(gen->arena, arena_strndup(gen->arena, param->name.start, param->name.length));
                            indented_fprintf(gen, 0, ", %s %s", param_type, param_name);
                        }
                        indented_fprintf(gen, 0, ");\n");
                    }
                }
                method_count++;
            }
        }
    }
    return method_count;
}

void code_gen_emit_struct_method_implementations(CodeGen *gen, Stmt **statements, int count)
{
    for (int i = 0; i < count; i++)
    {
        Stmt *stmt = statements[i];
        if (stmt->type == STMT_STRUCT_DECL)
        {
            StructDeclStmt *struct_decl = &stmt->as.struct_decl;
            char *struct_name = sn_mangle_name(gen->arena, arena_strndup(gen->arena, struct_decl->name.start, struct_decl->name.length));

            for (int j = 0; j < struct_decl->method_count; j++)
            {
                StructMethod *method = &struct_decl->methods[j];

                /* Skip native methods with no body - they are extern declared elsewhere */
                if (method->is_native && method->body == NULL)
                {
                    continue;
                }

                /* Resolve return type for native struct handle returns */
                Type *resolved_return_type = resolve_struct_type(gen, method->return_type);
                const char *ret_type;
                if (resolved_return_type != NULL &&
                    resolved_return_type->kind == TYPE_STRUCT &&
                    resolved_return_type->as.struct_type.is_native &&
                    resolved_return_type->as.struct_type.c_alias != NULL) {
                    ret_type = "RtHandleV2 *";
                } else {
                    ret_type = get_c_type(gen->arena, resolved_return_type);
                }

                /* Generate function signature */
                if (method->is_static)
                {
                    if (method->param_count == 0)
                    {
                        indented_fprintf(gen, 0, "%s %s_%s(RtArenaV2 *__caller_arena__) {\n",
                                         ret_type, struct_name, method->name);
                    }
                    else
                    {
                        indented_fprintf(gen, 0, "%s %s_%s(RtArenaV2 *__caller_arena__",
                                         ret_type, struct_name, method->name);
                        for (int k = 0; k < method->param_count; k++)
                        {
                            Parameter *param = &method->params[k];
                            const char *param_type = get_c_param_type(gen->arena, param->type);
                            char *param_name = sn_mangle_name(gen->arena, arena_strndup(gen->arena, param->name.start, param->name.length));
                            indented_fprintf(gen, 0, ", %s %s", param_type, param_name);
                        }
                        indented_fprintf(gen, 0, ") {\n");
                    }
                }
                else
                {
                    /* Instance method: first parameter is self (pointer to struct) */
                    /* For opaque handle types (native struct with c_alias), self is already a pointer */
                    if (struct_decl->is_native && struct_decl->c_alias != NULL)
                    {
                        /* Opaque handle: self type is the C alias pointer */
                        indented_fprintf(gen, 0, "%s %s_%s(RtArenaV2 *__caller_arena__, %s *__sn__self",
                                         ret_type, struct_name, method->name, struct_decl->c_alias);
                    }
                    else
                    {
                        /* Regular struct: self is pointer to struct */
                        indented_fprintf(gen, 0, "%s %s_%s(RtArenaV2 *__caller_arena__, %s *__sn__self",
                                         ret_type, struct_name, method->name, struct_name);
                    }
                    for (int k = 0; k < method->param_count; k++)
                    {
                        Parameter *param = &method->params[k];
                        const char *param_type = get_c_param_type(gen->arena, param->type);
                        char *param_name = sn_mangle_name(gen->arena, arena_strndup(gen->arena, param->name.start, param->name.length));
                        indented_fprintf(gen, 0, ", %s %s", param_type, param_name);
                    }
                    indented_fprintf(gen, 0, ") {\n");
                }

                /* Set up code generator state for method */
                char *method_full_name = arena_sprintf(gen->arena, "%s_%s", struct_name, method->name);
                char *saved_function = gen->current_function;
                Type *saved_return_type = gen->current_return_type;
                char *saved_arena_var = gen->current_arena_var;
                char *saved_function_arena = gen->function_arena_var;
                Scope *saved_function_scope = gen->function_scope;
                FunctionModifier saved_func_modifier = gen->current_func_modifier;
                bool saved_in_private = gen->in_private_context;
                bool saved_in_shared = gen->in_shared_context;
                int saved_temp_serial = gen->arena_temp_serial;
                int saved_temp_count = gen->arena_temp_count;

                gen->current_function = method_full_name;
                gen->current_return_type = method->return_type;

                /* Determine if this is an instance method on a regular (non-native, non-packed) struct */
                bool is_instance_method = (!method->is_static &&
                                           !struct_decl->is_native &&
                                           !struct_decl->is_packed);

                bool is_private = (method->modifier == FUNC_PRIVATE);
                bool is_shared = (method->modifier == FUNC_SHARED);

                if (is_instance_method)
                {
                    gen->current_arena_var = "__local_arena__";
                    gen->function_arena_var = "__local_arena__";
                    gen->current_func_modifier = method->modifier;
                    if (is_private) gen->in_private_context = true;
                    gen->in_shared_context = is_shared;
                }
                else
                {
                    gen->current_arena_var = "__caller_arena__";
                    gen->function_arena_var = "__caller_arena__";
                }
                gen->arena_temp_serial = 0;
                gen->arena_temp_count = 0;

                /* Push scope and add method params to symbol table for proper pinning */
                symbol_table_push_scope(gen->symbol_table);
                symbol_table_enter_arena(gen->symbol_table);
                gen->function_scope = gen->symbol_table->current;
                for (int k = 0; k < method->param_count; k++)
                {
                    symbol_table_add_symbol_full(gen->symbol_table, method->params[k].name,
                                                 method->params[k].type, SYMBOL_PARAM,
                                                 method->params[k].mem_qualifier);
                }

                /* Determine if we need a _return_value variable */
                bool has_return_value = (method->return_type && method->return_type->kind != TYPE_VOID);

                /* Add _return_value if needed */
                if (has_return_value)
                {
                    const char *default_val = get_default_value(method->return_type);
                    indented_fprintf(gen, 1, "%s _return_value = %s;\n", ret_type, default_val);
                }

                /* Emit arena setup for instance methods */
                if (is_instance_method)
                {
                    if (is_shared)
                    {
                        indented_fprintf(gen, 1, "RtArenaV2 *__local_arena__ = __sn__self->__arena__;\n");
                    }
                    else if (is_private)
                    {
                        indented_fprintf(gen, 1, "RtArenaV2 *__local_arena__ = rt_arena_v2_create(__sn__self->__arena__, RT_ARENA_MODE_PRIVATE, \"method\");\n");
                    }
                    else
                    {
                        /* DEFAULT */
                        indented_fprintf(gen, 1, "RtArenaV2 *__local_arena__ = rt_arena_v2_create(__sn__self->__arena__, RT_ARENA_MODE_DEFAULT, \"method\");\n");
                    }
                }

                /* Forward-declare variables that need cleanup at the return label.
                 * This ensures goto-based early returns don't leave them uninitialized. */
                code_gen_forward_declare_cleanup_vars(gen, method->body, method->body_count, 1);

                /* Generate method body */
                for (int k = 0; k < method->body_count; k++)
                {
                    if (method->body[k] != NULL)
                    {
                        code_gen_statement(gen, method->body[k], 1);
                    }
                }

                /* Add return label and cleanup */
                indented_fprintf(gen, 0, "%s_return:\n", method_full_name);
                code_gen_free_locals(gen, gen->symbol_table->current, true, 1);

                if (is_instance_method)
                {
                    /* Return value promotion FIRST — before self-field promotion.
                     * If the return value shares handles with self, promoting the return
                     * value first ensures those handles are cloned to caller_arena while
                     * still alive. Self-field promotion then safely re-promotes the
                     * (now dead) originals to self->__arena__.
                     * Target is __caller_arena__ — the arena guard (->arena == __local_arena__)
                     * ensures handles already on self->__arena__ are left untouched. */
                    if (has_return_value && !is_private)
                    {
                        code_gen_return_promotion(gen, method->return_type, false, is_shared, "__caller_arena__", 1);
                    }

                    if (!is_shared)
                    {
                        /* DEFAULT/PRIVATE: promote self handle fields before condemn */
                        code_gen_promote_self_fields(gen, struct_decl, 1);
                    }

                    if (!is_shared)
                    {
                        /* DEFAULT/PRIVATE: condemn the local arena */
                        indented_fprintf(gen, 1, "rt_arena_v2_condemn(__local_arena__);\n");
                    }
                }

                if (has_return_value)
                {
                    indented_fprintf(gen, 1, "return _return_value;\n");
                }
                else
                {
                    indented_fprintf(gen, 1, "return;\n");
                }

                /* Pop method scope */
                symbol_table_pop_scope(gen->symbol_table);

                /* Restore code generator state */
                gen->current_function = saved_function;
                gen->current_return_type = saved_return_type;
                gen->current_arena_var = saved_arena_var;
                gen->function_arena_var = saved_function_arena;
                gen->function_scope = saved_function_scope;
                gen->current_func_modifier = saved_func_modifier;
                gen->in_private_context = saved_in_private;
                gen->in_shared_context = saved_in_shared;
                gen->arena_temp_serial = saved_temp_serial;
                gen->arena_temp_count = saved_temp_count;

                /* Close function */
                indented_fprintf(gen, 0, "}\n\n");
            }
        }
    }
}
