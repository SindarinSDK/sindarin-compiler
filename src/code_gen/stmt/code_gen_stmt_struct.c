/*
 * code_gen_stmt_struct.c - Struct method code generation
 *
 * Handles code generation for struct instance and static methods.
 */

#include "code_gen/stmt/code_gen_stmt.h"
#include "code_gen/stmt/code_gen_stmt_func_promote.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void code_gen_struct_methods(CodeGen *gen, StructDeclStmt *struct_decl, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_struct_methods");

    /* Track already-emitted struct methods to avoid duplicates.
     * This can happen when the same module is imported via different namespaces. */
    char *raw_struct_name = arena_strndup(gen->arena, struct_decl->name.start, struct_decl->name.length);
    char *struct_name = sn_mangle_name(gen->arena, raw_struct_name);

    /* Check if this struct's methods have already been emitted */
    for (int i = 0; i < gen->emitted_struct_methods_count; i++)
    {
        if (strcmp(gen->emitted_struct_methods[i], struct_name) == 0)
        {
            /* Already emitted, skip */
            return;
        }
    }

    /* Mark this struct as emitted */
    if (gen->emitted_struct_methods_count >= gen->emitted_struct_methods_capacity)
    {
        int new_capacity = gen->emitted_struct_methods_capacity == 0 ? 8 : gen->emitted_struct_methods_capacity * 2;
        const char **new_array = arena_alloc(gen->arena, sizeof(const char *) * new_capacity);
        for (int i = 0; i < gen->emitted_struct_methods_count; i++)
        {
            new_array[i] = gen->emitted_struct_methods[i];
        }
        gen->emitted_struct_methods = new_array;
        gen->emitted_struct_methods_capacity = new_capacity;
    }
    gen->emitted_struct_methods[gen->emitted_struct_methods_count++] = struct_name;

    for (int j = 0; j < struct_decl->method_count; j++)
    {
        StructMethod *method = &struct_decl->methods[j];

        /* Skip native methods with no body - they are extern declared elsewhere */
        if (method->is_native && method->body == NULL)
        {
            continue;
        }

        /* Resolve return type (may be forward-declared struct without c_alias) */
        Type *resolved_return_type = resolve_struct_type(gen, method->return_type);
        const char *ret_type = get_c_type(gen->arena, resolved_return_type);

        /* Generate function signature */
        if (method->is_static)
        {
            if (method->param_count == 0)
            {
                indented_fprintf(gen, indent, "%s %s_%s(RtArenaV2 *__caller_arena__) {\n",
                                 ret_type, struct_name, method->name);
            }
            else
            {
                indented_fprintf(gen, indent, "%s %s_%s(RtArenaV2 *__caller_arena__",
                                 ret_type, struct_name, method->name);
                for (int k = 0; k < method->param_count; k++)
                {
                    Parameter *param = &method->params[k];
                    Type *resolved_param_type = resolve_struct_type(gen, param->type);
                    const char *param_type = get_c_param_type(gen->arena, resolved_param_type);
                    char *param_name = sn_mangle_name(gen->arena, arena_strndup(gen->arena, param->name.start, param->name.length));
                    indented_fprintf(gen, 0, ", %s %s", param_type, param_name);
                }
                indented_fprintf(gen, 0, ") {\n");
            }
        }
        else
        {
            /* Instance method: first parameter is self (pointer to struct) */
            if (struct_decl->is_native && struct_decl->c_alias != NULL)
            {
                /* Opaque handle: self type is the C alias pointer */
                indented_fprintf(gen, indent, "%s %s_%s(RtArenaV2 *__caller_arena__, %s *__sn__self",
                                 ret_type, struct_name, method->name, struct_decl->c_alias);
            }
            else
            {
                /* Regular struct: self is pointer to struct */
                indented_fprintf(gen, indent, "%s %s_%s(RtArenaV2 *__caller_arena__, %s *__sn__self",
                                 ret_type, struct_name, method->name, struct_name);
            }
            for (int k = 0; k < method->param_count; k++)
            {
                Parameter *param = &method->params[k];
                Type *resolved_param_type = resolve_struct_type(gen, param->type);
                const char *param_type = get_c_param_type(gen->arena, resolved_param_type);
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
            indented_fprintf(gen, indent + 1, "%s _return_value = %s;\n", ret_type, default_val);
        }

        /* Emit arena setup for instance methods */
        if (is_instance_method)
        {
            if (is_shared)
            {
                indented_fprintf(gen, indent + 1, "RtArenaV2 *__local_arena__ = __sn__self->__arena__;\n");
            }
            else if (is_private)
            {
                indented_fprintf(gen, indent + 1, "RtArenaV2 *__local_arena__ = rt_arena_v2_create(__sn__self->__arena__, RT_ARENA_MODE_PRIVATE, \"method\");\n");
            }
            else
            {
                /* DEFAULT */
                indented_fprintf(gen, indent + 1, "RtArenaV2 *__local_arena__ = rt_arena_v2_create(__sn__self->__arena__, RT_ARENA_MODE_DEFAULT, \"method\");\n");
            }
        }

        /* Forward-declare variables that need cleanup at the return label.
         * This ensures goto-based early returns don't leave them uninitialized. */
        code_gen_forward_declare_cleanup_vars(gen, method->body, method->body_count, indent + 1);

        /* Generate method body */
        for (int k = 0; k < method->body_count; k++)
        {
            if (method->body[k] != NULL)
            {
                code_gen_statement(gen, method->body[k], indent + 1);
            }
        }

        /* Add return label and cleanup */
        indented_fprintf(gen, indent, "%s_return:\n", method_full_name);
        code_gen_free_locals(gen, gen->symbol_table->current, true, indent + 1);

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
                code_gen_return_promotion(gen, method->return_type, false, is_shared, "__caller_arena__", indent + 1);
            }

            if (!is_shared)
            {
                /* DEFAULT/PRIVATE: promote self handle fields before condemn */
                code_gen_promote_self_fields(gen, struct_decl, indent + 1);
            }

            if (!is_shared)
            {
                /* DEFAULT/PRIVATE: condemn the local arena */
                indented_fprintf(gen, indent + 1, "rt_arena_v2_condemn(__local_arena__);\n");
            }
        }

        if (has_return_value)
        {
            indented_fprintf(gen, indent + 1, "return _return_value;\n");
        }
        else
        {
            indented_fprintf(gen, indent + 1, "return;\n");
        }

        /* Pop method scope */
        symbol_table_pop_scope(gen->symbol_table);

        /* Restore code generator state */
        gen->current_function = saved_function;
        gen->current_return_type = saved_return_type;
        gen->current_arena_var = saved_arena_var;
        gen->function_arena_var = saved_function_arena;
        gen->current_func_modifier = saved_func_modifier;
        gen->in_private_context = saved_in_private;
        gen->in_shared_context = saved_in_shared;
        gen->arena_temp_serial = saved_temp_serial;
        gen->arena_temp_count = saved_temp_count;

        /* Close function */
        indented_fprintf(gen, indent, "}\n\n");
    }
}
