/**
 * code_gen_method_emit.c - Struct method emission for code generation
 *
 * Contains functions for emitting struct method forward declarations
 * and implementations during code generation.
 */

#include "code_gen/emit/code_gen_method_emit.h"
#include "code_gen/util/code_gen_util.h"
#include "code_gen/stmt/code_gen_stmt.h"
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

                const char *ret_type = get_c_type(gen->arena, method->return_type);

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

                gen->current_function = method_full_name;
                gen->current_return_type = method->return_type;
                gen->current_arena_var = "__caller_arena__";
                gen->function_arena_var = "__caller_arena__";

                /* Push scope and add method params to symbol table for proper pinning */
                symbol_table_push_scope(gen->symbol_table);
                symbol_table_enter_arena(gen->symbol_table);
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

                /* Generate method body */
                for (int k = 0; k < method->body_count; k++)
                {
                    if (method->body[k] != NULL)
                    {
                        code_gen_statement(gen, method->body[k], 1);
                    }
                }

                /* Add return label and return statement */
                indented_fprintf(gen, 0, "%s_return:\n", method_full_name);
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

                /* Close function */
                indented_fprintf(gen, 0, "}\n\n");
            }
        }
    }
}
