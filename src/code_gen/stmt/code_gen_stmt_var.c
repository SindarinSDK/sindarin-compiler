/*
 * code_gen_stmt_var.c - Variable declaration code generation
 *
 * Handles code generation for variable declarations including thread spawns,
 * memory qualifiers (as ref, as val), and global variable deferred initialization.
 */

#include "code_gen/stmt/code_gen_stmt.h"
#include "code_gen/stmt/code_gen_stmt_var_init.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include "symbol_table/symbol_table_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Threshold for stack vs heap allocation for structs.
 * Structs smaller than this are stack-allocated.
 * Structs >= this size are heap-allocated via rt_arena_v2_alloc.
 * This matches the same threshold used for fixed arrays. */
#define STRUCT_STACK_THRESHOLD 8192  /* 8KB */

void code_gen_var_declaration(CodeGen *gen, VarDeclStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_var_declaration");

    char *raw_var_name = get_var_name(gen->arena, stmt->name);

    /* Detect global scope: no current arena means we're at file scope */
    bool is_global_scope = (gen->current_arena_var == NULL);

    /* Static prefix for module-level static variables */
    const char *static_prefix = (stmt->is_static && is_global_scope) ? "static " : "";

    /* If we're generating code for an imported namespace AND this is a global variable,
     * prefix the variable name with the appropriate namespace to avoid collisions. */
    char *var_name;
    if (is_global_scope)
    {
        const char *prefix_to_use = NULL;
        if (stmt->is_static && gen->current_canonical_module != NULL)
        {
            prefix_to_use = gen->current_canonical_module;
        }
        else if (gen->current_namespace_prefix != NULL)
        {
            prefix_to_use = gen->current_namespace_prefix;
        }

        if (prefix_to_use != NULL)
        {
            char *prefixed_name = arena_sprintf(gen->arena, "%s__%s", prefix_to_use, raw_var_name);
            var_name = sn_mangle_name(gen->arena, prefixed_name);
        }
        else
        {
            var_name = sn_mangle_name(gen->arena, raw_var_name);
        }
    }
    else
    {
        var_name = sn_mangle_name(gen->arena, raw_var_name);
    }

    /* Check for duplicate global emission */
    if (is_global_scope)
    {
        for (int i = 0; i < gen->emitted_globals_count; i++)
        {
            if (strcmp(gen->emitted_globals[i], var_name) == 0)
            {
                DEBUG_VERBOSE("Skipping duplicate global: %s", var_name);
                return;
            }
        }
        /* Track this global */
        if (gen->emitted_globals_count >= gen->emitted_globals_capacity)
        {
            int new_capacity = gen->emitted_globals_capacity == 0 ? 16 : gen->emitted_globals_capacity * 2;
            const char **new_array = arena_alloc(gen->arena, sizeof(const char *) * new_capacity);
            for (int i = 0; i < gen->emitted_globals_count; i++)
            {
                new_array[i] = gen->emitted_globals[i];
            }
            gen->emitted_globals = new_array;
            gen->emitted_globals_capacity = new_capacity;
        }
        gen->emitted_globals[gen->emitted_globals_count++] = arena_strdup(gen->arena, var_name);
    }

    /* Track static globals */
    if (stmt->is_static && is_global_scope)
    {
        bool found_in_static = false;
        for (int i = 0; i < gen->emitted_static_globals_count; i++)
        {
            if (strcmp(gen->emitted_static_globals[i], var_name) == 0)
            {
                found_in_static = true;
                break;
            }
        }
        if (!found_in_static)
        {
            if (gen->emitted_static_globals_count >= gen->emitted_static_globals_capacity)
            {
                int new_capacity = gen->emitted_static_globals_capacity == 0 ? 8 : gen->emitted_static_globals_capacity * 2;
                const char **new_array = arena_alloc(gen->arena, sizeof(const char *) * new_capacity);
                for (int i = 0; i < gen->emitted_static_globals_count; i++)
                {
                    new_array[i] = gen->emitted_static_globals[i];
                }
                gen->emitted_static_globals = new_array;
                gen->emitted_static_globals_capacity = new_capacity;
            }
            gen->emitted_static_globals[gen->emitted_static_globals_count++] = arena_strdup(gen->arena, var_name);
        }
    }

    /* Handle global empty arrays */
    if (is_global_scope && stmt->type->kind == TYPE_ARRAY)
    {
        bool is_empty = (stmt->initializer == NULL);
        if (!is_empty && stmt->initializer->type == EXPR_ARRAY)
        {
            is_empty = (stmt->initializer->as.array.element_count == 0);
        }

        if (is_empty)
        {
            const char *type_c = get_c_type(gen->arena, stmt->type);
            symbol_table_add_symbol_full(gen->symbol_table, stmt->name, stmt->type, SYMBOL_GLOBAL, stmt->mem_qualifier);
            if (stmt->sync_modifier == SYNC_ATOMIC)
            {
                Symbol *sym = symbol_table_lookup_symbol_current(gen->symbol_table, stmt->name);
                if (sym != NULL) sym->sync_mod = SYNC_ATOMIC;
            }
            if (stmt->has_pending_elements)
            {
                Symbol *sym = symbol_table_lookup_symbol_current(gen->symbol_table, stmt->name);
                if (sym != NULL) sym->has_pending_elements = true;
            }
            indented_fprintf(gen, indent, "%s%s %s = NULL;\n", static_prefix, type_c, var_name);
            /* Emit companion pending elements array for thread spawn push tracking */
            if (stmt->has_pending_elements)
            {
                indented_fprintf(gen, indent, "%sRtHandleV2 *__%s_pending_elems__ = NULL;\n", static_prefix, raw_var_name);
            }
            return;
        }
    }

    /* Check for thread spawn */
    bool is_thread_spawn = (stmt->initializer != NULL && stmt->initializer->type == EXPR_THREAD_SPAWN);
    bool is_primitive_type = (stmt->type->kind == TYPE_INT ||
                              stmt->type->kind == TYPE_LONG ||
                              stmt->type->kind == TYPE_DOUBLE ||
                              stmt->type->kind == TYPE_BOOL ||
                              stmt->type->kind == TYPE_BYTE ||
                              stmt->type->kind == TYPE_CHAR);
    bool is_spawn_handle_result = gen->current_arena_var != NULL &&
                                  (stmt->type->kind == TYPE_STRING ||
                                   (stmt->type->kind == TYPE_ARRAY && !is_any_element_array_type(stmt->type)));
    bool is_struct_result = (stmt->type->kind == TYPE_STRUCT);
    bool needs_pending_var = is_primitive_type || is_spawn_handle_result || is_struct_result;

    const char *type_c = get_c_type(gen->arena, stmt->type);

    /* For types that could be thread spawn results, declare pending variable */
    bool has_special_mem_qual = (stmt->mem_qualifier == MEM_AS_REF || stmt->mem_qualifier == MEM_AS_VAL);
    bool is_captured_by_ref = code_gen_is_captured_primitive(gen, raw_var_name) &&
                              (is_primitive_type || stmt->type->kind == TYPE_ARRAY);

    if (needs_pending_var && !is_global_scope && !has_special_mem_qual && !is_captured_by_ref)
    {
        char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);

        if (is_thread_spawn)
        {
            char *init_str = code_gen_expression(gen, stmt->initializer);
            indented_fprintf(gen, indent, "RtHandleV2 *%s = %s;\n", pending_var, init_str);
            indented_fprintf(gen, indent, "%s %s;\n", type_c, var_name);
        }
        else
        {
            indented_fprintf(gen, indent, "RtHandleV2 *%s = NULL;\n", pending_var);
            if (stmt->initializer)
            {
                bool prev_as_handle = gen->expr_as_handle;
                if (is_spawn_handle_result)
                {
                    gen->expr_as_handle = true;
                }
                char *init_str = code_gen_expression(gen, stmt->initializer);
                gen->expr_as_handle = prev_as_handle;
                indented_fprintf(gen, indent, "%s %s = %s;\n", type_c, var_name, init_str);
            }
            else
            {
                indented_fprintf(gen, indent, "%s %s;\n", type_c, var_name);
            }
        }

        symbol_table_add_symbol_full(gen->symbol_table, stmt->name, stmt->type, SYMBOL_LOCAL, stmt->mem_qualifier);
        if (stmt->sync_modifier == SYNC_ATOMIC)
        {
            Symbol *sym = symbol_table_lookup_symbol_current(gen->symbol_table, stmt->name);
            if (sym != NULL) sym->sync_mod = SYNC_ATOMIC;
        }
        /* Emit companion pending elements array for arrays with thread spawn push */
        if (stmt->has_pending_elements && stmt->type->kind == TYPE_ARRAY)
        {
            indented_fprintf(gen, indent, "RtHandleV2 *__%s_pending_elems__ = NULL;\n", raw_var_name);
            Symbol *sym = symbol_table_lookup_symbol_current(gen->symbol_table, stmt->name);
            if (sym != NULL) sym->has_pending_elements = true;
        }
        return;
    }

    /* Check for captured primitives */
    MemoryQualifier effective_qual = stmt->mem_qualifier;
    if (effective_qual == MEM_DEFAULT && code_gen_is_captured_primitive(gen, raw_var_name))
    {
        effective_qual = MEM_AS_REF;
    }

    /* Add to symbol table */
    SymbolKind sym_kind = is_global_scope ? SYMBOL_GLOBAL : SYMBOL_LOCAL;
    symbol_table_add_symbol_full(gen->symbol_table, stmt->name, stmt->type, sym_kind, effective_qual);
    {
        Symbol *sym = symbol_table_lookup_symbol_current(gen->symbol_table, stmt->name);
        if (sym != NULL && stmt->sync_modifier == SYNC_ATOMIC)
        {
            sym->sync_mod = SYNC_ATOMIC;
        }
    }

    /* Generate initializer expression */
    char *init_str;
    if (stmt->initializer)
    {
        if (stmt->initializer->type == EXPR_LAMBDA)
        {
            gen->current_decl_var_name = raw_var_name;
            gen->recursive_lambda_id = -1;
        }

        bool prev_as_handle = gen->expr_as_handle;
        if (!is_global_scope && gen->current_arena_var != NULL)
        {
            /* V2 clone functions take handles, so enable handle mode for as val too */
            if (is_handle_type(stmt->type))
            {
                gen->expr_as_handle = true;
            }
            else if (stmt->type->kind == TYPE_ANY && stmt->initializer != NULL &&
                     stmt->initializer->expr_type != NULL &&
                     stmt->initializer->expr_type->kind == TYPE_ARRAY)
            {
                gen->expr_as_handle = true;
            }
        }

        /* Handle deferred global initialization */
        char *saved_arena_var = gen->current_arena_var;
        if (is_global_scope && gen->current_arena_var == NULL)
        {
            bool will_need_deferred = is_handle_type(stmt->type) ||
                (stmt->type->kind == TYPE_STRUCT && stmt->initializer != NULL &&
                 (stmt->initializer->type == EXPR_CALL || stmt->initializer->type == EXPR_METHOD_CALL));
            if (will_need_deferred)
            {
                gen->current_arena_var = "__main_arena__";
                gen->expr_as_handle = is_handle_type(stmt->type);
            }
        }

        init_str = code_gen_expression(gen, stmt->initializer);

        gen->current_arena_var = saved_arena_var;
        gen->expr_as_handle = prev_as_handle;

        /* Handle string parameter copy */
        if (!is_global_scope && gen->current_arena_var != NULL &&
            stmt->type->kind == TYPE_STRING && stmt->mem_qualifier != MEM_AS_VAL &&
            stmt->initializer != NULL && stmt->initializer->type == EXPR_VARIABLE)
        {
            Symbol *init_sym = symbol_table_lookup_symbol(gen->symbol_table, stmt->initializer->as.variable.name);
            if (init_sym != NULL && init_sym->kind == SYMBOL_PARAM)
            {
                init_str = arena_sprintf(gen->arena,
                    "rt_arena_v2_clone(%s, %s)",
                    ARENA_VAR(gen), init_str);
            }
        }

        /* Handle deferred global init */
        bool needs_deferred_init = false;
        if (is_global_scope)
        {
            if (is_handle_type(stmt->type))
            {
                needs_deferred_init = true;
            }
            else if (stmt->initializer != NULL &&
                     (stmt->initializer->type == EXPR_CALL || stmt->initializer->type == EXPR_METHOD_CALL))
            {
                needs_deferred_init = true;
            }
        }

        if (needs_deferred_init)
        {
            char *deferred_value = arena_strdup(gen->arena, init_str);

            if (gen->deferred_global_count >= gen->deferred_global_capacity)
            {
                int new_cap = gen->deferred_global_capacity == 0 ? 8 : gen->deferred_global_capacity * 2;
                char **new_names = arena_alloc(gen->arena, new_cap * sizeof(char *));
                char **new_values = arena_alloc(gen->arena, new_cap * sizeof(char *));
                for (int i = 0; i < gen->deferred_global_count; i++)
                {
                    new_names[i] = gen->deferred_global_names[i];
                    new_values[i] = gen->deferred_global_values[i];
                }
                gen->deferred_global_names = new_names;
                gen->deferred_global_values = new_values;
                gen->deferred_global_capacity = new_cap;
            }
            gen->deferred_global_names[gen->deferred_global_count] = var_name;
            gen->deferred_global_values[gen->deferred_global_count] = deferred_value;
            gen->deferred_global_count++;

            if (is_handle_type(stmt->type))
            {
                init_str = arena_strdup(gen->arena, "NULL");
            }
            else if (stmt->type->kind == TYPE_STRUCT)
            {
                if (stmt->type->as.struct_type.is_native && stmt->type->as.struct_type.c_alias != NULL)
                {
                    init_str = arena_strdup(gen->arena, "NULL");
                }
                else
                {
                    init_str = arena_strdup(gen->arena, "{0}");
                }
            }
            else
            {
                init_str = arena_strdup(gen->arena, "0");
            }
        }

        /* Handle boxing to 'any' */
        if (stmt->type->kind == TYPE_ANY && stmt->initializer->expr_type != NULL &&
            stmt->initializer->expr_type->kind != TYPE_ANY)
        {
            init_str = code_gen_box_value(gen, init_str, stmt->initializer->expr_type);
        }

        /* Handle array-to-any conversion */
        if (stmt->type->kind == TYPE_ARRAY && stmt->initializer->expr_type != NULL)
        {
            init_str = code_gen_var_array_conversion(gen, stmt->type, stmt->initializer->expr_type, init_str);
        }

        /* Handle 'as val' cloning */
        if (stmt->mem_qualifier == MEM_AS_VAL)
        {
            if (stmt->type->kind == TYPE_ARRAY)
            {
                Type *elem_type = stmt->type->as.array.element_type;
                /* V2 clone: strings need special handling, others use generic */
                if (elem_type->kind == TYPE_STRING) {
                    init_str = arena_sprintf(gen->arena, "rt_array_clone_string_v2(%s)", init_str);
                } else {
                    const char *sizeof_expr = get_c_sizeof_elem(gen->arena, elem_type);
                    init_str = arena_sprintf(gen->arena, "rt_array_clone_v2(%s, %s)", init_str, sizeof_expr);
                }
            }
            else if (stmt->type->kind == TYPE_STRING)
            {
                init_str = arena_sprintf(gen->arena, "rt_arena_v2_strdup(%s, %s)", ARENA_VAR(gen), init_str);
            }
        }
    }
    else
    {
        init_str = arena_strdup(gen->arena, get_default_value(stmt->type));
    }

    /* Handle 'as ref' - heap allocate */
    if (effective_qual == MEM_AS_REF)
    {
        bool in_main = (gen->current_function != NULL && strcmp(gen->current_function, "main") == 0);
        const char *alloc_arena = (gen->allocate_closure_in_caller_arena &&
                                   strcmp(gen->current_arena_var, "__local_arena__") == 0 && !in_main)
            ? "__caller_arena__"
            : ARENA_VAR(gen);
        indented_fprintf(gen, indent, "RtHandleV2 *__%s_h__ = rt_arena_v2_alloc(%s, sizeof(%s));\n",
                         var_name, alloc_arena, type_c);
        indented_fprintf(gen, indent, "%s *%s = (%s *)__%s_h__->ptr;\n",
                         type_c, var_name, type_c, var_name);
        indented_fprintf(gen, indent, "*%s = %s;\n", var_name, init_str);
    }
    /* Handle large struct heap allocation */
    else if (stmt->type->kind == TYPE_STRUCT && gen->current_arena_var != NULL)
    {
        int struct_size = (int)stmt->type->as.struct_type.size;
        if (struct_size == 0 && stmt->type->as.struct_type.name != NULL)
        {
            Token struct_name_token = {
                .start = stmt->type->as.struct_type.name,
                .length = (int)strlen(stmt->type->as.struct_type.name)
            };
            Symbol *struct_sym = symbol_table_lookup_type(gen->symbol_table, struct_name_token);
            if (struct_sym != NULL && struct_sym->type != NULL && struct_sym->type->kind == TYPE_STRUCT)
            {
                struct_size = (int)struct_sym->type->as.struct_type.size;
            }
        }
        if (struct_size >= STRUCT_STACK_THRESHOLD)
        {
            indented_fprintf(gen, indent, "RtHandleV2 *__%s_h__ = rt_arena_v2_alloc(%s, sizeof(%s));\n",
                             var_name, ARENA_VAR(gen), type_c);
            indented_fprintf(gen, indent, "%s *%s = (%s *)__%s_h__->ptr;\n",
                             type_c, var_name, type_c, var_name);
            indented_fprintf(gen, indent, "*%s = %s;\n", var_name, init_str);

            Symbol *sym = symbol_table_lookup_symbol_current(gen->symbol_table, stmt->name);
            if (sym != NULL)
            {
                sym->mem_qual = MEM_AS_REF;
            }
        }
        else
        {
            indented_fprintf(gen, indent, "%s%s %s = %s;\n", static_prefix, type_c, var_name, init_str);
        }
    }
    else
    {
        indented_fprintf(gen, indent, "%s%s %s = %s;\n", static_prefix, type_c, var_name, init_str);
    }

    /* Emit companion pending elements array for arrays with thread spawn push tracking */
    if (stmt->has_pending_elements && stmt->type->kind == TYPE_ARRAY && !is_global_scope)
    {
        indented_fprintf(gen, indent, "RtHandleV2 *__%s_pending_elems__ = NULL;\n", raw_var_name);
        Symbol *sym = symbol_table_lookup_symbol_current(gen->symbol_table, stmt->name);
        if (sym != NULL) sym->has_pending_elements = true;
    }

    /* Handle recursive lambda fixup */
    if (gen->recursive_lambda_id >= 0 && stmt->initializer != NULL &&
        stmt->initializer->type == EXPR_LAMBDA)
    {
        int lambda_id = gen->recursive_lambda_id;
        indented_fprintf(gen, indent, "((__closure_%d__ *)%s)->%s = %s;\n",
                         lambda_id, var_name, raw_var_name, var_name);
        gen->recursive_lambda_id = -1;
    }

    gen->current_decl_var_name = NULL;
}
