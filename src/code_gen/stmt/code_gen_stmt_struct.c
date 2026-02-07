/*
 * code_gen_stmt_struct.c - Struct method code generation
 *
 * Handles code generation for struct instance and static methods.
 */

#include "code_gen/stmt/code_gen_stmt.h"
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
            indented_fprintf(gen, indent + 1, "%s _return_value = %s;\n", ret_type, default_val);
        }

        /* Generate method body */
        for (int k = 0; k < method->body_count; k++)
        {
            if (method->body[k] != NULL)
            {
                code_gen_statement(gen, method->body[k], indent + 1);
            }
        }

        /* Add return label and return statement */
        indented_fprintf(gen, indent, "%s_return:\n", method_full_name);
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

        /* Close function */
        indented_fprintf(gen, indent, "}\n\n");
    }
}
