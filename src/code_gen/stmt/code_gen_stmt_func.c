/*
 * code_gen_stmt_func.c - Function code generation
 *
 * Handles code generation for function declarations including parameter handling,
 * arena setup, tail call optimization, and cleanup.
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

void code_gen_function(CodeGen *gen, FunctionStmt *stmt)
{
    DEBUG_VERBOSE("Entering code_gen_function");

    /* Skip native functions without body */
    if (stmt->is_native && stmt->body_count == 0)
    {
        return;
    }

    /* Save state */
    char *old_function = gen->current_function;
    Type *old_return_type = gen->current_return_type;
    FunctionModifier old_func_modifier = gen->current_func_modifier;
    bool old_in_private_context = gen->in_private_context;
    bool old_in_shared_context = gen->in_shared_context;
    char *old_arena_var = gen->current_arena_var;
    int old_arena_depth = gen->arena_depth;

    char *raw_fn_name = get_var_name(gen->arena, stmt->name);
    bool is_main = strcmp(raw_fn_name, "main") == 0;

    /* Build function name with namespace prefix if needed */
    if (is_main || stmt->is_native)
    {
        gen->current_function = raw_fn_name;
    }
    else if (gen->current_namespace_prefix != NULL)
    {
        char prefixed_name[512];
        snprintf(prefixed_name, sizeof(prefixed_name), "%s__%s", gen->current_namespace_prefix, raw_fn_name);
        gen->current_function = sn_mangle_name(gen->arena, prefixed_name);
    }
    else
    {
        gen->current_function = sn_mangle_name(gen->arena, raw_fn_name);
    }

    /* Check for duplicate emission */
    for (int i = 0; i < gen->emitted_functions_count; i++)
    {
        if (strcmp(gen->emitted_functions[i], gen->current_function) == 0)
        {
            gen->current_function = old_function;
            return;
        }
    }

    /* Track function as emitted */
    if (gen->emitted_functions_count >= gen->emitted_functions_capacity)
    {
        int new_capacity = gen->emitted_functions_capacity == 0 ? 16 : gen->emitted_functions_capacity * 2;
        const char **new_array = arena_alloc(gen->arena, sizeof(const char *) * new_capacity);
        for (int i = 0; i < gen->emitted_functions_count; i++)
        {
            new_array[i] = gen->emitted_functions[i];
        }
        gen->emitted_functions = new_array;
        gen->emitted_functions_capacity = new_capacity;
    }
    gen->emitted_functions[gen->emitted_functions_count++] = arena_strdup(gen->arena, gen->current_function);

    gen->current_return_type = stmt->return_type;
    gen->current_func_modifier = stmt->modifier;
    bool main_has_args = is_main && stmt->param_count == 1;
    bool is_private = stmt->modifier == FUNC_PRIVATE;
    bool is_shared = stmt->modifier == FUNC_SHARED;

    /* Setup arena context */
    if (is_private) gen->in_private_context = true;
    gen->in_shared_context = is_shared;
    gen->current_arena_var = "__local_arena__";
    gen->function_arena_var = "__local_arena__";

    const char *ret_c = is_main ? "int" : get_c_type(gen->arena, gen->current_return_type);
    bool has_return_value = (gen->current_return_type && gen->current_return_type->kind != TYPE_VOID) || is_main;

    symbol_table_push_scope(gen->symbol_table);
    symbol_table_enter_arena(gen->symbol_table);

    for (int i = 0; i < stmt->param_count; i++)
    {
        symbol_table_add_symbol_full(gen->symbol_table, stmt->params[i].name, stmt->params[i].type,
                                     SYMBOL_PARAM, stmt->params[i].mem_qualifier);
    }

    /* Scan for captured primitives */
    code_gen_scan_captured_primitives(gen, stmt->body, stmt->body_count);

    /* Emit function signature */
    indented_fprintf(gen, 0, "%s %s(", ret_c, gen->current_function);

    if (main_has_args)
    {
        fprintf(gen->output, "int argc, char **argv");
    }
    else
    {
        if (!is_main)
        {
            fprintf(gen->output, "RtArenaV2 *__caller_arena__");
            if (stmt->param_count > 0)
                fprintf(gen->output, ", ");
        }

        for (int i = 0; i < stmt->param_count; i++)
        {
            const char *param_type_c = get_c_param_type(gen->arena, stmt->params[i].type);
            char *param_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, stmt->params[i].name));

            bool is_ref_param = false;
            if (stmt->params[i].mem_qualifier == MEM_AS_REF && stmt->params[i].type != NULL)
            {
                TypeKind kind = stmt->params[i].type->kind;
                bool is_prim = (kind == TYPE_INT || kind == TYPE_INT32 || kind == TYPE_UINT ||
                               kind == TYPE_UINT32 || kind == TYPE_LONG || kind == TYPE_DOUBLE ||
                               kind == TYPE_FLOAT || kind == TYPE_CHAR || kind == TYPE_BOOL ||
                               kind == TYPE_BYTE);
                bool is_struct = (kind == TYPE_STRUCT);
                is_ref_param = is_prim || is_struct;
            }

            if (is_ref_param)
                fprintf(gen->output, "%s *%s", param_type_c, param_name);
            else
                fprintf(gen->output, "%s %s", param_type_c, param_name);

            if (i < stmt->param_count - 1)
                fprintf(gen->output, ", ");
        }
    }
    indented_fprintf(gen, 0, ") {\n");

    /* Setup local arena */
    if (is_main)
    {
        indented_fprintf(gen, 1, "RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, \"main\");\n");
        indented_fprintf(gen, 1, "__main_arena__ = __local_arena__;\n");
        for (int i = 0; i < gen->deferred_global_count; i++)
        {
            indented_fprintf(gen, 1, "%s = %s;\n",
                             gen->deferred_global_names[i], gen->deferred_global_values[i]);
        }
    }
    else if (is_shared)
    {
        indented_fprintf(gen, 1, "RtArenaV2 *__local_arena__ = __caller_arena__;\n");
    }
    else
    {
        indented_fprintf(gen, 1, "RtArenaV2 *__local_arena__ = rt_arena_v2_create(__caller_arena__, RT_ARENA_MODE_PRIVATE, \"func\");\n");
    }

    /* Clone handle-type parameters */
    if (!is_main && !is_shared && !main_has_args)
    {
        for (int i = 0; i < stmt->param_count; i++)
        {
            Type *param_type = stmt->params[i].type;
            if (param_type == NULL) continue;

            if (param_type->kind == TYPE_STRING)
            {
                char *param_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, stmt->params[i].name));
                indented_fprintf(gen, 1, "%s = rt_arena_v2_clone(__local_arena__, %s);\n",
                                 param_name, param_name);
                Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, stmt->params[i].name);
                if (sym) sym->kind = SYMBOL_LOCAL;
            }
            else if (param_type->kind == TYPE_STRUCT && stmt->params[i].mem_qualifier != MEM_AS_REF)
            {
                int field_count = param_type->as.struct_type.field_count;
                char *param_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, stmt->params[i].name));
                for (int f = 0; f < field_count; f++)
                {
                    StructField *field = &param_type->as.struct_type.fields[f];
                    if (field->type && field->type->kind == TYPE_STRING)
                    {
                        const char *c_field_name = field->c_alias != NULL ? field->c_alias : sn_mangle_name(gen->arena, field->name);
                        indented_fprintf(gen, 1, "%s.%s = rt_arena_v2_clone(__local_arena__, %s.%s);\n",
                                         param_name, c_field_name, param_name, c_field_name);
                    }
                }
            }
        }
    }

    /* Declare return value */
    if (has_return_value)
    {
        const char *default_val = is_main ? "0" : get_default_value(gen->current_return_type);
        indented_fprintf(gen, 1, "%s _return_value = %s;\n", ret_c, default_val);
    }

    /* Handle main with args */
    if (main_has_args)
    {
        char *param_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, stmt->params[0].name));
        indented_fprintf(gen, 1, "RtHandleV2 *%s = rt_args_create_v2(%s, argc, argv);\n",
                         param_name, gen->current_arena_var);
        Symbol *args_sym = symbol_table_lookup_symbol(gen->symbol_table, stmt->params[0].name);
        if (args_sym) args_sym->kind = SYMBOL_LOCAL;
    }

    /* Clone 'as val' parameters */
    for (int i = 0; i < stmt->param_count; i++)
    {
        if (stmt->params[i].mem_qualifier == MEM_AS_VAL)
        {
            Type *param_type = stmt->params[i].type;
            char *param_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, stmt->params[i].name));

            if (param_type->kind == TYPE_ARRAY)
            {
                Type *elem_type = param_type->as.array.element_type;
                const char *suffix = code_gen_type_suffix(elem_type);
                /* Clone the handle-based array to get a local copy */
                indented_fprintf(gen, 1, "%s = rt_array_clone_%s_v2(%s);\n",
                                 param_name, suffix, param_name);
                Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, stmt->params[i].name);
                if (sym) sym->kind = SYMBOL_LOCAL;
            }
            else if (param_type->kind == TYPE_STRING)
            {
                indented_fprintf(gen, 1, "%s = rt_to_string_string(%s, %s);\n",
                                 param_name, ARENA_VAR(gen), param_name);
                Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, stmt->params[i].name);
                if (sym) sym->kind = SYMBOL_LOCAL;
            }
        }
    }

    /* Check for tail calls */
    bool has_tail_calls = function_has_marked_tail_calls(stmt);
    bool old_in_tail_call_function = gen->in_tail_call_function;
    FunctionStmt *old_tail_call_fn = gen->tail_call_fn;

    if (has_tail_calls)
    {
        gen->in_tail_call_function = true;
        gen->tail_call_fn = stmt;
        indented_fprintf(gen, 1, "while (1) { /* tail call loop */\n");
    }

    bool has_return = (stmt->body_count > 0 && stmt->body[stmt->body_count - 1]->type == STMT_RETURN);

    /* Set closure allocation flag */
    bool old_allocate_closure_in_caller_arena = gen->allocate_closure_in_caller_arena;
    if (!is_main && stmt->return_type && stmt->return_type->kind == TYPE_FUNCTION)
    {
        gen->allocate_closure_in_caller_arena = true;
    }

    /* Generate body */
    int body_indent = has_tail_calls ? 2 : 1;
    for (int i = 0; i < stmt->body_count; i++)
    {
        code_gen_statement(gen, stmt->body[i], body_indent);
    }

    gen->allocate_closure_in_caller_arena = old_allocate_closure_in_caller_arena;

    if (!has_return)
    {
        indented_fprintf(gen, body_indent, "goto %s_return;\n", gen->current_function);
    }

    if (has_tail_calls)
    {
        indented_fprintf(gen, 1, "} /* end tail call loop */\n");
    }

    gen->in_tail_call_function = old_in_tail_call_function;
    gen->tail_call_fn = old_tail_call_fn;

    /* Return label and cleanup */
    indented_fprintf(gen, 0, "%s_return:\n", gen->current_function);
    code_gen_free_locals(gen, gen->symbol_table->current, true, 1);

    /* Promote return value if needed */
    if (has_return_value)
    {
        code_gen_return_promotion(gen, stmt->return_type, is_main, is_shared, 1);
    }

    /* Destroy arena */
    if (is_main)
    {
        indented_fprintf(gen, 1, "rt_arena_v2_destroy(__local_arena__);\n");
    }
    else if (!is_shared)
    {
        indented_fprintf(gen, 1, "rt_arena_v2_destroy(__local_arena__);\n");
    }

    /* Return statement */
    if (has_return_value)
    {
        indented_fprintf(gen, 1, "return _return_value;\n");
    }
    else
    {
        indented_fprintf(gen, 1, "return;\n");
    }
    indented_fprintf(gen, 0, "}\n\n");

    symbol_table_exit_arena(gen->symbol_table);
    symbol_table_pop_scope(gen->symbol_table);
    code_gen_clear_captured_primitives(gen);

    /* Restore state */
    gen->current_function = old_function;
    gen->current_return_type = old_return_type;
    gen->current_func_modifier = old_func_modifier;
    gen->in_private_context = old_in_private_context;
    gen->in_shared_context = old_in_shared_context;
    gen->current_arena_var = old_arena_var;
    gen->arena_depth = old_arena_depth;
}
