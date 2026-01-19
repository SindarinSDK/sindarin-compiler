#include "code_gen/code_gen_stmt.h"
#include "code_gen/code_gen_stmt_loop.h"
#include "code_gen/code_gen_stmt_capture.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include "symbol_table/symbol_table_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Threshold for stack vs heap allocation for structs.
 * Structs smaller than this are stack-allocated.
 * Structs >= this size are heap-allocated via rt_arena_alloc.
 * This matches the same threshold used for fixed arrays. */
#define STRUCT_STACK_THRESHOLD 8192  /* 8KB */

/* Generate thread sync as a statement - assigns results back to variables
 * For single sync (r!): r = sync_result
 * For sync list ([r1, r2, r3]!): r1 = sync_result1; r2 = sync_result2; ... */
static void code_gen_thread_sync_statement(CodeGen *gen, Expr *expr, int indent)
{
    ThreadSyncExpr *sync = &expr->as.thread_sync;

    if (sync->is_array)
    {
        /* Sync list: [r1, r2, r3]!
         * Generate sync and assignment for each variable */
        Expr *list_expr = sync->handle;
        if (list_expr->type != EXPR_SYNC_LIST)
        {
            fprintf(stderr, "Error: Multi-sync requires sync list expression\n");
            exit(1);
        }

        SyncListExpr *sync_list = &list_expr->as.sync_list;

        if (sync_list->element_count == 0)
        {
            /* Empty sync list - no-op */
            return;
        }

        /* Sync each variable and assign result back */
        for (int i = 0; i < sync_list->element_count; i++)
        {
            Expr *elem = sync_list->elements[i];
            if (elem->type != EXPR_VARIABLE)
            {
                fprintf(stderr, "Error: Sync list elements must be variables\n");
                exit(1);
            }

            char *var_name = get_var_name(gen->arena, elem->as.variable.name);

            /* Look up the variable's type from the symbol table
             * The elem->expr_type may not be set for array elements */
            Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, elem->as.variable.name);
            Type *result_type = (sym != NULL) ? sym->type : elem->expr_type;

            /* Check if void - just sync, no assignment */
            if (result_type == NULL || result_type->kind == TYPE_VOID)
            {
                indented_fprintf(gen, indent, "rt_thread_sync(%s);\n", var_name);
                continue;
            }

            const char *c_type = get_c_type(gen->arena, result_type);
            const char *rt_type = get_rt_result_type(result_type);

            bool is_primitive = (result_type->kind == TYPE_INT ||
                                result_type->kind == TYPE_LONG ||
                                result_type->kind == TYPE_DOUBLE ||
                                result_type->kind == TYPE_BOOL ||
                                result_type->kind == TYPE_BYTE ||
                                result_type->kind == TYPE_CHAR);

            if (is_primitive)
            {
                /* For primitives, we declared two variables: __var_pending__ (RtThreadHandle*)
                 * and var (actual type). Sync the pending handle and assign to the typed var.
                 * Pattern: var = *(type *)sync(__var_pending__, ...) */
                char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", var_name);
                indented_fprintf(gen, indent, "%s = *(%s *)rt_thread_sync_with_result(%s, %s, %s);\n",
                    var_name, c_type, pending_var, ARENA_VAR(gen), rt_type);
            }
            else
            {
                /* For reference types (arrays, strings), direct assignment works
                 * because both are pointer types */
                indented_fprintf(gen, indent, "%s = (%s)rt_thread_sync_with_result(%s, %s, %s);\n",
                    var_name, c_type, var_name, ARENA_VAR(gen), rt_type);
            }
        }
    }
    else
    {
        /* Single sync: r!
         * Only assign back if the handle is a variable */
        Expr *handle = sync->handle;

        if (handle->type == EXPR_VARIABLE)
        {
            char *var_name = get_var_name(gen->arena, handle->as.variable.name);
            Type *result_type = expr->expr_type;

            /* Check if void - just sync, no assignment */
            if (result_type == NULL || result_type->kind == TYPE_VOID)
            {
                indented_fprintf(gen, indent, "rt_thread_sync(%s);\n", var_name);
                return;
            }

            const char *c_type = get_c_type(gen->arena, result_type);
            const char *rt_type = get_rt_result_type(result_type);

            bool is_primitive = (result_type->kind == TYPE_INT ||
                                result_type->kind == TYPE_LONG ||
                                result_type->kind == TYPE_DOUBLE ||
                                result_type->kind == TYPE_BOOL ||
                                result_type->kind == TYPE_BYTE ||
                                result_type->kind == TYPE_CHAR);

            if (is_primitive)
            {
                /* For primitives, we declared two variables: __var_pending__ (RtThreadHandle*)
                 * and var (actual type). Sync the pending handle and assign to the typed var.
                 * Pattern: var = *(type *)sync(__var_pending__, ...) */
                char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", var_name);
                indented_fprintf(gen, indent, "%s = *(%s *)rt_thread_sync_with_result(%s, %s, %s);\n",
                    var_name, c_type, pending_var, ARENA_VAR(gen), rt_type);
            }
            else
            {
                /* For reference types (arrays, strings), direct assignment works
                 * because both are pointer types */
                indented_fprintf(gen, indent, "%s = (%s)rt_thread_sync_with_result(%s, %s, %s);\n",
                    var_name, c_type, var_name, ARENA_VAR(gen), rt_type);
            }
        }
        else
        {
            /* Non-variable sync (e.g., &fn()!) - just execute the sync expression */
            char *expr_str = code_gen_expression(gen, expr);
            indented_fprintf(gen, indent, "%s;\n", expr_str);
        }
    }
}

void code_gen_expression_statement(CodeGen *gen, ExprStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_expression_statement");

    /* Special handling for thread sync statements - need to assign results back to variables */
    if (stmt->expression->type == EXPR_THREAD_SYNC)
    {
        code_gen_thread_sync_statement(gen, stmt->expression, indent);
        return;
    }

    char *expr_str = code_gen_expression(gen, stmt->expression);
    if (stmt->expression->expr_type->kind == TYPE_STRING && expression_produces_temp(stmt->expression))
    {
        // Skip freeing in arena context - arena handles cleanup
        if (gen->current_arena_var == NULL)
        {
            indented_fprintf(gen, indent, "{\n");
            indented_fprintf(gen, indent + 1, "char *_tmp = %s;\n", expr_str);
            indented_fprintf(gen, indent + 1, "(void)_tmp;\n");
            indented_fprintf(gen, indent + 1, "rt_free_string(_tmp);\n");
            indented_fprintf(gen, indent, "}\n");
        }
        else
        {
            indented_fprintf(gen, indent, "%s;\n", expr_str);
        }
    }
    else if (stmt->expression->type == EXPR_CALL && stmt->expression->expr_type->kind == TYPE_VOID)
    {
        // Statement expressions need a semicolon after them
        indented_fprintf(gen, indent, "%s;\n", expr_str);
    }
    else
    {
        indented_fprintf(gen, indent, "%s;\n", expr_str);
    }
}

void code_gen_var_declaration(CodeGen *gen, VarDeclStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_var_declaration");
    char *var_name = get_var_name(gen->arena, stmt->name);

    // Detect global scope: no current arena means we're at file scope
    // Global arrays with empty initializers must be initialized to NULL since C
    // doesn't allow function calls or compound literals in global initializers.
    // Arrays with actual values need runtime initialization (handled separately).
    bool is_global_scope = (gen->current_arena_var == NULL);
    if (is_global_scope && stmt->type->kind == TYPE_ARRAY)
    {
        // Check if this is an empty initializer or no initializer
        bool is_empty = (stmt->initializer == NULL);
        if (!is_empty && stmt->initializer->type == EXPR_ARRAY)
        {
            is_empty = (stmt->initializer->as.array.element_count == 0);
        }

        if (is_empty)
        {
            const char *type_c = get_c_type(gen->arena, stmt->type);
            symbol_table_add_symbol_full(gen->symbol_table, stmt->name, stmt->type, SYMBOL_LOCAL, stmt->mem_qualifier);
            /* Set sync modifier if present */
            if (stmt->sync_modifier == SYNC_ATOMIC)
            {
                Symbol *sym = symbol_table_lookup_symbol_current(gen->symbol_table, stmt->name);
                if (sym != NULL) sym->sync_mod = SYNC_ATOMIC;
            }
            indented_fprintf(gen, indent, "%s %s = NULL;\n", type_c, var_name);
            return;
        }
        // Non-empty global arrays will fall through and get the function call initializer,
        // which may cause C compile errors. This is a known limitation - global arrays
        // with values should be avoided or initialized in main().
    }

    // Check if this is a thread spawn assignment.
    // For thread spawns with primitive types, we declare TWO variables:
    //   1. __varname_pending__ of type RtThreadHandle* to hold the handle
    //   2. varname of the actual type to hold the result after sync
    // For reference types (arrays, strings), we use the actual type directly
    // since both handles and results are pointer types.
    bool is_thread_spawn = (stmt->initializer != NULL &&
                            stmt->initializer->type == EXPR_THREAD_SPAWN);
    bool is_primitive_type = (stmt->type->kind == TYPE_INT ||
                              stmt->type->kind == TYPE_LONG ||
                              stmt->type->kind == TYPE_DOUBLE ||
                              stmt->type->kind == TYPE_BOOL ||
                              stmt->type->kind == TYPE_BYTE ||
                              stmt->type->kind == TYPE_CHAR);

    const char *type_c = get_c_type(gen->arena, stmt->type);

    // For thread spawn with primitive result, generate two declarations
    if (is_thread_spawn && is_primitive_type)
    {
        char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", var_name);
        char *init_str = code_gen_expression(gen, stmt->initializer);

        // Declare the pending handle variable
        indented_fprintf(gen, indent, "RtThreadHandle *%s = %s;\n", pending_var, init_str);

        // Declare the actual typed variable (uninitialized, will be set on sync)
        indented_fprintf(gen, indent, "%s %s;\n", type_c, var_name);

        // Add to symbol table
        symbol_table_add_symbol_full(gen->symbol_table, stmt->name, stmt->type, SYMBOL_LOCAL, stmt->mem_qualifier);
        /* Set sync modifier if present */
        if (stmt->sync_modifier == SYNC_ATOMIC)
        {
            Symbol *sym = symbol_table_lookup_symbol_current(gen->symbol_table, stmt->name);
            if (sym != NULL) sym->sync_mod = SYNC_ATOMIC;
        }
        return;
    }

    // Check if this primitive is captured by a closure - if so, treat it like 'as ref'
    // This ensures mutations inside closures are visible to the outer scope
    MemoryQualifier effective_qual = stmt->mem_qualifier;
    if (effective_qual == MEM_DEFAULT && code_gen_is_captured_primitive(gen, var_name))
    {
        effective_qual = MEM_AS_REF;
    }

    // Add to symbol table with effective qualifier so accesses are dereferenced correctly
    symbol_table_add_symbol_full(gen->symbol_table, stmt->name, stmt->type, SYMBOL_LOCAL, effective_qual);
    /* Set sync modifier if present */
    if (stmt->sync_modifier == SYNC_ATOMIC)
    {
        Symbol *sym = symbol_table_lookup_symbol_current(gen->symbol_table, stmt->name);
        if (sym != NULL) sym->sync_mod = SYNC_ATOMIC;
    }

    char *init_str;
    if (stmt->initializer)
    {
        /* For lambda initializers, track the variable name so we can detect recursive lambdas */
        if (stmt->initializer->type == EXPR_LAMBDA)
        {
            gen->current_decl_var_name = var_name;
            gen->recursive_lambda_id = -1;  /* Will be set by lambda codegen if recursive */
        }

        init_str = code_gen_expression(gen, stmt->initializer);
        // Wrap string literals in rt_to_string_string to create heap-allocated copies
        // This is needed because string variables may be freed/reassigned later
        if (stmt->type->kind == TYPE_STRING && stmt->initializer->type == EXPR_LITERAL)
        {
            init_str = arena_sprintf(gen->arena, "rt_to_string_string(%s, %s)", ARENA_VAR(gen), init_str);
        }

        // Handle boxing when assigning to 'any' type
        // If the variable is 'any' and initializer is a concrete type, wrap with boxing function
        if (stmt->type->kind == TYPE_ANY && stmt->initializer->expr_type != NULL &&
            stmt->initializer->expr_type->kind != TYPE_ANY)
        {
            init_str = code_gen_box_value(gen, init_str, stmt->initializer->expr_type);
        }

        // Handle conversion when assigning typed array to any[], any[][], or any[][][]
        if (stmt->type->kind == TYPE_ARRAY &&
            stmt->type->as.array.element_type != NULL &&
            stmt->initializer->expr_type != NULL &&
            stmt->initializer->expr_type->kind == TYPE_ARRAY &&
            stmt->initializer->expr_type->as.array.element_type != NULL)
        {
            Type *decl_elem = stmt->type->as.array.element_type;
            Type *src_elem = stmt->initializer->expr_type->as.array.element_type;

            // Check for 3D array: any[][][] = T[][][]
            if (decl_elem->kind == TYPE_ARRAY &&
                decl_elem->as.array.element_type != NULL &&
                decl_elem->as.array.element_type->kind == TYPE_ARRAY &&
                decl_elem->as.array.element_type->as.array.element_type != NULL &&
                decl_elem->as.array.element_type->as.array.element_type->kind == TYPE_ANY &&
                src_elem->kind == TYPE_ARRAY &&
                src_elem->as.array.element_type != NULL &&
                src_elem->as.array.element_type->kind == TYPE_ARRAY &&
                src_elem->as.array.element_type->as.array.element_type != NULL &&
                src_elem->as.array.element_type->as.array.element_type->kind != TYPE_ANY)
            {
                Type *innermost_src = src_elem->as.array.element_type->as.array.element_type;
                const char *conv_func = NULL;
                switch (innermost_src->kind)
                {
                case TYPE_INT:
                case TYPE_INT32:
                case TYPE_UINT:
                case TYPE_UINT32:
                case TYPE_LONG:
                    conv_func = "rt_array3_to_any_long";
                    break;
                case TYPE_DOUBLE:
                case TYPE_FLOAT:
                    conv_func = "rt_array3_to_any_double";
                    break;
                case TYPE_CHAR:
                    conv_func = "rt_array3_to_any_char";
                    break;
                case TYPE_BOOL:
                    conv_func = "rt_array3_to_any_bool";
                    break;
                case TYPE_BYTE:
                    conv_func = "rt_array3_to_any_byte";
                    break;
                case TYPE_STRING:
                    conv_func = "rt_array3_to_any_string";
                    break;
                default:
                    break;
                }
                if (conv_func != NULL)
                {
                    init_str = arena_sprintf(gen->arena, "%s(%s, %s)", conv_func, ARENA_VAR(gen), init_str);
                }
            }
            // Check for 2D array: any[][] = T[][]
            else if (decl_elem->kind == TYPE_ARRAY &&
                decl_elem->as.array.element_type != NULL &&
                decl_elem->as.array.element_type->kind == TYPE_ANY &&
                src_elem->kind == TYPE_ARRAY &&
                src_elem->as.array.element_type != NULL &&
                src_elem->as.array.element_type->kind != TYPE_ANY)
            {
                Type *inner_src = src_elem->as.array.element_type;
                const char *conv_func = NULL;
                switch (inner_src->kind)
                {
                case TYPE_INT:
                case TYPE_INT32:
                case TYPE_UINT:
                case TYPE_UINT32:
                case TYPE_LONG:
                    conv_func = "rt_array2_to_any_long";
                    break;
                case TYPE_DOUBLE:
                case TYPE_FLOAT:
                    conv_func = "rt_array2_to_any_double";
                    break;
                case TYPE_CHAR:
                    conv_func = "rt_array2_to_any_char";
                    break;
                case TYPE_BOOL:
                    conv_func = "rt_array2_to_any_bool";
                    break;
                case TYPE_BYTE:
                    conv_func = "rt_array2_to_any_byte";
                    break;
                case TYPE_STRING:
                    conv_func = "rt_array2_to_any_string";
                    break;
                default:
                    break;
                }
                if (conv_func != NULL)
                {
                    init_str = arena_sprintf(gen->arena, "%s(%s, %s)", conv_func, ARENA_VAR(gen), init_str);
                }
            }
            // Check for 1D array: any[] = T[]
            else if (decl_elem->kind == TYPE_ANY && src_elem->kind != TYPE_ANY)
            {
                const char *conv_func = NULL;
                switch (src_elem->kind)
                {
                case TYPE_INT:
                case TYPE_INT32:
                case TYPE_UINT:
                case TYPE_UINT32:
                case TYPE_LONG:
                    conv_func = "rt_array_to_any_long";
                    break;
                case TYPE_DOUBLE:
                case TYPE_FLOAT:
                    conv_func = "rt_array_to_any_double";
                    break;
                case TYPE_CHAR:
                    conv_func = "rt_array_to_any_char";
                    break;
                case TYPE_BOOL:
                    conv_func = "rt_array_to_any_bool";
                    break;
                case TYPE_BYTE:
                    conv_func = "rt_array_to_any_byte";
                    break;
                case TYPE_STRING:
                    conv_func = "rt_array_to_any_string";
                    break;
                default:
                    break;
                }
                if (conv_func != NULL)
                {
                    init_str = arena_sprintf(gen->arena, "%s(%s, %s)", conv_func, ARENA_VAR(gen), init_str);
                }
            }
        }

        // Handle 'as val' - create a copy for arrays and strings
        if (stmt->mem_qualifier == MEM_AS_VAL)
        {
            if (stmt->type->kind == TYPE_ARRAY)
            {
                // Get element type suffix for the clone function
                Type *elem_type = stmt->type->as.array.element_type;
                const char *suffix = code_gen_type_suffix(elem_type);
                init_str = arena_sprintf(gen->arena, "rt_array_clone_%s(%s, %s)", suffix, ARENA_VAR(gen), init_str);
            }
            else if (stmt->type->kind == TYPE_STRING)
            {
                init_str = arena_sprintf(gen->arena, "rt_to_string_string(%s, %s)", ARENA_VAR(gen), init_str);
            }
        }
    }
    else
    {
        init_str = arena_strdup(gen->arena, get_default_value(stmt->type));
    }

    // Handle 'as ref' or captured primitives - heap-allocate via arena
    if (effective_qual == MEM_AS_REF)
    {
        // Allocate on arena and store pointer
        // e.g., long *x = (long *)rt_arena_alloc(__arena_1__, sizeof(long)); *x = 42L;
        // When the function returns a closure type, allocate in caller's arena so
        // the captured data survives the function's local arena destruction.
        // Exception: main() has no caller, so always use local arena.
        bool in_main = (gen->current_function != NULL && strcmp(gen->current_function, "main") == 0);
        const char *alloc_arena = (gen->allocate_closure_in_caller_arena &&
                                   strcmp(gen->current_arena_var, "__local_arena__") == 0 &&
                                   !in_main)
            ? "__caller_arena__"
            : ARENA_VAR(gen);
        indented_fprintf(gen, indent, "%s *%s = (%s *)rt_arena_alloc(%s, sizeof(%s));\n",
                         type_c, var_name, type_c, alloc_arena, type_c);
        indented_fprintf(gen, indent, "*%s = %s;\n", var_name, init_str);
    }
    // Handle large struct allocation (>= 8KB threshold) - heap-allocate via arena
    else if (stmt->type->kind == TYPE_STRUCT && gen->current_arena_var != NULL)
    {
        // Get the struct size - try from the type itself first, otherwise look up from symbol table
        int struct_size = (int)stmt->type->as.struct_type.size;
        if (struct_size == 0 && stmt->type->as.struct_type.name != NULL)
        {
            // Look up the struct type from the symbol table which has the computed layout
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
            // Large struct: allocate on arena and store as pointer
            // e.g., LargeStruct *s = (LargeStruct *)rt_arena_alloc(__arena_1__, sizeof(LargeStruct));
            //       *s = (LargeStruct){ .field = value, ... };
            indented_fprintf(gen, indent, "%s *%s = (%s *)rt_arena_alloc(%s, sizeof(%s));\n",
                             type_c, var_name, type_c, ARENA_VAR(gen), type_c);
            indented_fprintf(gen, indent, "*%s = %s;\n", var_name, init_str);

            // Update symbol table to mark as pointer for proper access
            Symbol *sym = symbol_table_lookup_symbol_current(gen->symbol_table, stmt->name);
            if (sym != NULL)
            {
                sym->mem_qual = MEM_AS_REF;  // Mark as pointer for proper dereferencing
            }
        }
        else
        {
            // Small struct: stack allocation with value semantics
            indented_fprintf(gen, indent, "%s %s = %s;\n", type_c, var_name, init_str);
        }
    }
    else
    {
        indented_fprintf(gen, indent, "%s %s = %s;\n", type_c, var_name, init_str);
    }

    /* For recursive lambdas, we need to fix up the self-reference after declaration.
     * The lambda's closure was created without the self-capture to avoid using
     * an uninitialized variable. Now that the variable is initialized, we can
     * set the self-reference in the closure. */
    if (gen->recursive_lambda_id >= 0 && stmt->initializer != NULL &&
        stmt->initializer->type == EXPR_LAMBDA)
    {
        int lambda_id = gen->recursive_lambda_id;
        /* Generate: ((__closure_N__ *)var)->var = var; */
        indented_fprintf(gen, indent, "((__closure_%d__ *)%s)->%s = %s;\n",
                         lambda_id, var_name, var_name, var_name);
        gen->recursive_lambda_id = -1;
    }

    /* Clear the current decl var name */
    gen->current_decl_var_name = NULL;
}

void code_gen_free_locals(CodeGen *gen, Scope *scope, bool is_function, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_free_locals");

    // Skip manual freeing when in arena context - arena handles all deallocation
    if (gen->current_arena_var != NULL)
    {
        return;
    }

    Symbol *sym = scope->symbols;
    while (sym)
    {
        if (sym->type && sym->type->kind == TYPE_STRING && sym->kind == SYMBOL_LOCAL)
        {
            char *var_name = get_var_name(gen->arena, sym->name);
            indented_fprintf(gen, indent, "if (%s) {\n", var_name);
            if (is_function && gen->current_return_type && gen->current_return_type->kind == TYPE_STRING)
            {
                indented_fprintf(gen, indent + 1, "if (%s != _return_value) {\n", var_name);
                indented_fprintf(gen, indent + 2, "rt_free_string(%s);\n", var_name);
                indented_fprintf(gen, indent + 1, "}\n");
            }
            else
            {
                indented_fprintf(gen, indent + 1, "rt_free_string(%s);\n", var_name);
            }
            indented_fprintf(gen, indent, "}\n");
        }
        else if (sym->type && sym->type->kind == TYPE_ARRAY && sym->kind == SYMBOL_LOCAL)
        {
            char *var_name = get_var_name(gen->arena, sym->name);
            Type *elem_type = sym->type->as.array.element_type;
            indented_fprintf(gen, indent, "if (%s) {\n", var_name);
            if (is_function && gen->current_return_type && gen->current_return_type->kind == TYPE_ARRAY)
            {
                indented_fprintf(gen, indent + 1, "if (%s != _return_value) {\n", var_name);
                if (elem_type && elem_type->kind == TYPE_STRING)
                {
                    indented_fprintf(gen, indent + 2, "rt_array_free_string(%s);\n", var_name);
                }
                else
                {
                    indented_fprintf(gen, indent + 2, "rt_array_free(%s);\n", var_name);
                }
                indented_fprintf(gen, indent + 1, "}\n");
            }
            else
            {
                if (elem_type && elem_type->kind == TYPE_STRING)
                {
                    indented_fprintf(gen, indent + 1, "rt_array_free_string(%s);\n", var_name);
                }
                else
                {
                    indented_fprintf(gen, indent + 1, "rt_array_free(%s);\n", var_name);
                }
            }
            indented_fprintf(gen, indent, "}\n");
        }
        sym = sym->next;
    }
}

void code_gen_block(CodeGen *gen, BlockStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_block");

    bool old_in_shared_context = gen->in_shared_context;
    bool old_in_private_context = gen->in_private_context;
    char *old_arena_var = gen->current_arena_var;
    int old_arena_depth = gen->arena_depth;

    bool is_shared = stmt->modifier == BLOCK_SHARED;
    bool is_private = stmt->modifier == BLOCK_PRIVATE;

    symbol_table_push_scope(gen->symbol_table);

    // Handle private block - create new arena
    if (is_private)
    {
        gen->in_private_context = true;
        gen->in_shared_context = false;
        gen->arena_depth++;
        gen->current_arena_var = arena_sprintf(gen->arena, "__arena_%d__", gen->arena_depth);
        /* Push arena name to stack for tracking nested private blocks */
        push_arena_to_stack(gen, gen->current_arena_var);
        symbol_table_enter_arena(gen->symbol_table);
    }
    // Handle shared block - uses parent's arena
    else if (is_shared)
    {
        gen->in_shared_context = true;
    }

    indented_fprintf(gen, indent, "{\n");

    // For private blocks, create a local arena
    if (is_private)
    {
        indented_fprintf(gen, indent + 1, "RtArena *%s = rt_arena_create(NULL);\n", gen->current_arena_var);
    }

    for (int i = 0; i < stmt->count; i++)
    {
        code_gen_statement(gen, stmt->statements[i], indent + 1);
    }
    code_gen_free_locals(gen, gen->symbol_table->current, false, indent + 1);

    // For private blocks, destroy the arena
    if (is_private)
    {
        indented_fprintf(gen, indent + 1, "rt_arena_destroy(%s);\n", gen->current_arena_var);
        symbol_table_exit_arena(gen->symbol_table);
        /* Pop arena name from stack */
        pop_arena_from_stack(gen);
    }

    indented_fprintf(gen, indent, "}\n");
    symbol_table_pop_scope(gen->symbol_table);

    // Restore context
    gen->in_shared_context = old_in_shared_context;
    gen->in_private_context = old_in_private_context;
    gen->current_arena_var = old_arena_var;
    gen->arena_depth = old_arena_depth;
}

void code_gen_function(CodeGen *gen, FunctionStmt *stmt)
{
    DEBUG_VERBOSE("Entering code_gen_function");

    /* Native functions without a body are external C declarations.
     * We don't generate any code - they must be provided via #pragma include
     * or linked via #pragma link. */
    if (stmt->is_native && stmt->body_count == 0)
    {
        DEBUG_VERBOSE("Skipping native function without body: %.*s", stmt->name.length, stmt->name.start);
        return;
    }

    char *old_function = gen->current_function;
    Type *old_return_type = gen->current_return_type;
    FunctionModifier old_func_modifier = gen->current_func_modifier;
    bool old_in_private_context = gen->in_private_context;
    bool old_in_shared_context = gen->in_shared_context;
    char *old_arena_var = gen->current_arena_var;
    int old_arena_depth = gen->arena_depth;

    gen->current_function = get_var_name(gen->arena, stmt->name);
    gen->current_return_type = stmt->return_type;
    gen->current_func_modifier = stmt->modifier;

    bool is_main = strcmp(gen->current_function, "main") == 0;
    bool main_has_args = is_main && stmt->param_count == 1;  // Type checker validated it's str[]
    bool is_private = stmt->modifier == FUNC_PRIVATE;
    bool is_shared = stmt->modifier == FUNC_SHARED;

    // New arena model: ALL non-main functions receive __caller_arena__ as first parameter.
    // The modifier determines how the function uses it:
    //   shared:  __local_arena__ = __caller_arena__ (alias, no new arena)
    //   default: __local_arena__ = rt_arena_create(__caller_arena__) (new arena with parent)
    //   private: __local_arena__ = rt_arena_create(__caller_arena__) (new arena, strict escape)
    //
    // main() is special - it creates the root arena with no caller.
    //
    // For default functions returning heap types, the return value is promoted to
    // __caller_arena__ before __local_arena__ is destroyed.

    // Set up arena context - all functions use __local_arena__
    if (is_private) gen->in_private_context = true;
    gen->in_shared_context = is_shared;
    gen->current_arena_var = "__local_arena__";

    // Special case for main: always use "int" return type in C for standard entry point.
    const char *ret_c = is_main ? "int" : get_c_type(gen->arena, gen->current_return_type);
    // Determine if we need a _return_value variable: only for non-void or main.
    bool has_return_value = (gen->current_return_type && gen->current_return_type->kind != TYPE_VOID) || is_main;
    symbol_table_push_scope(gen->symbol_table);

    // All functions have arena context
    symbol_table_enter_arena(gen->symbol_table);

    for (int i = 0; i < stmt->param_count; i++)
    {
        /* Pass memory qualifier so code gen knows about 'as ref' parameters */
        symbol_table_add_symbol_full(gen->symbol_table, stmt->params[i].name, stmt->params[i].type,
                                     SYMBOL_PARAM, stmt->params[i].mem_qualifier);
    }

    // Pre-pass: scan function body for primitives captured by closures
    // These need to be declared as pointers for mutation persistence
    code_gen_scan_captured_primitives(gen, stmt->body, stmt->body_count);

    indented_fprintf(gen, 0, "%s %s(", ret_c, gen->current_function);

    // Main with args gets special C signature: int main(int argc, char **argv)
    if (main_has_args)
    {
        fprintf(gen->output, "int argc, char **argv");
    }
    else
    {
        // All non-main functions receive caller's arena as first parameter
        if (!is_main)
        {
            fprintf(gen->output, "RtArena *__caller_arena__");
            if (stmt->param_count > 0)
            {
                fprintf(gen->output, ", ");
            }
        }

        for (int i = 0; i < stmt->param_count; i++)
        {
            const char *param_type_c = get_c_type(gen->arena, stmt->params[i].type);
            char *param_name = get_var_name(gen->arena, stmt->params[i].name);

            /* 'as ref' primitive and struct parameters become pointer types */
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
            {
                fprintf(gen->output, "%s *%s", param_type_c, param_name);
            }
            else
            {
                fprintf(gen->output, "%s %s", param_type_c, param_name);
            }

            if (i < stmt->param_count - 1)
            {
                fprintf(gen->output, ", ");
            }
        }
    }
    indented_fprintf(gen, 0, ") {\n");

    // Set up __local_arena__ based on modifier:
    //   main:    create root arena (no parent)
    //   shared:  alias to caller's arena
    //   default: new arena with caller as parent
    //   private: new arena with caller as parent (strict escape rules enforced at compile time)
    if (is_main)
    {
        indented_fprintf(gen, 1, "RtArena *__local_arena__ = rt_arena_create(NULL);\n");
    }
    else if (is_shared)
    {
        indented_fprintf(gen, 1, "RtArena *__local_arena__ = __caller_arena__;\n");
    }
    else
    {
        // default or private - create new arena with caller as parent
        indented_fprintf(gen, 1, "RtArena *__local_arena__ = rt_arena_create(__caller_arena__);\n");
    }

    // Add _return_value only if needed (non-void or main).
    if (has_return_value)
    {
        const char *default_val = is_main ? "0" : get_default_value(gen->current_return_type);
        indented_fprintf(gen, 1, "%s _return_value = %s;\n", ret_c, default_val);
    }

    // Initialize args array for main if it has parameters
    if (main_has_args)
    {
        char *param_name = get_var_name(gen->arena, stmt->params[0].name);
        indented_fprintf(gen, 1, "char **%s = rt_args_create(%s, argc, argv);\n",
                         param_name, gen->current_arena_var);
    }

    // Clone 'as val' array parameters to ensure copy semantics
    for (int i = 0; i < stmt->param_count; i++)
    {
        if (stmt->params[i].mem_qualifier == MEM_AS_VAL)
        {
            Type *param_type = stmt->params[i].type;
            if (param_type->kind == TYPE_ARRAY)
            {
                char *param_name = get_var_name(gen->arena, stmt->params[i].name);
                Type *elem_type = param_type->as.array.element_type;
                const char *suffix = code_gen_type_suffix(elem_type);
                indented_fprintf(gen, 1, "%s = rt_array_clone_%s(%s, %s);\n",
                                 param_name, suffix, ARENA_VAR(gen), param_name);
            }
            else if (param_type->kind == TYPE_STRING)
            {
                char *param_name = get_var_name(gen->arena, stmt->params[i].name);
                indented_fprintf(gen, 1, "%s = rt_to_string_string(%s, %s);\n",
                                 param_name, ARENA_VAR(gen), param_name);
            }
        }
    }

    // Check if function has marked tail calls for optimization
    bool has_tail_calls = function_has_marked_tail_calls(stmt);

    // Set up tail call optimization state
    bool old_in_tail_call_function = gen->in_tail_call_function;
    FunctionStmt *old_tail_call_fn = gen->tail_call_fn;

    if (has_tail_calls)
    {
        gen->in_tail_call_function = true;
        gen->tail_call_fn = stmt;
        // Wrap function body in a loop for tail call optimization
        indented_fprintf(gen, 1, "while (1) { /* tail call loop */\n");
    }

    bool has_return = false;
    if (stmt->body_count > 0 && stmt->body[stmt->body_count - 1]->type == STMT_RETURN)
    {
        has_return = true;
    }

    // If the function returns a closure type, set the flag so all closures
    // created in this function are allocated in the caller's arena.
    // This handles the pattern where a closure is stored in a variable
    // before being returned.
    bool old_allocate_closure_in_caller_arena = gen->allocate_closure_in_caller_arena;
    if (!is_main && stmt->return_type && stmt->return_type->kind == TYPE_FUNCTION)
    {
        gen->allocate_closure_in_caller_arena = true;
    }

    int body_indent = has_tail_calls ? 2 : 1;
    for (int i = 0; i < stmt->body_count; i++)
    {
        code_gen_statement(gen, stmt->body[i], body_indent);
    }

    // Restore the flag
    gen->allocate_closure_in_caller_arena = old_allocate_closure_in_caller_arena;
    if (!has_return)
    {
        indented_fprintf(gen, body_indent, "goto %s_return;\n", gen->current_function);
    }

    if (has_tail_calls)
    {
        indented_fprintf(gen, 1, "} /* end tail call loop */\n");
    }

    // Restore tail call state
    gen->in_tail_call_function = old_in_tail_call_function;
    gen->tail_call_fn = old_tail_call_fn;

    indented_fprintf(gen, 0, "%s_return:\n", gen->current_function);
    code_gen_free_locals(gen, gen->symbol_table->current, true, 1);

    // For non-main, non-shared functions with heap return types, promote the return
    // value to the caller's arena before destroying the local arena.
    bool needs_promotion = !is_main && !is_shared && has_return_value && stmt->return_type;
    if (needs_promotion)
    {
        TypeKind kind = stmt->return_type->kind;
        if (kind == TYPE_STRING)
        {
            indented_fprintf(gen, 1, "_return_value = rt_arena_promote_string(__caller_arena__, _return_value);\n");
        }
        else if (kind == TYPE_ARRAY)
        {
            // Clone array to caller's arena (effectively promotes it)
            Type *elem_type = stmt->return_type->as.array.element_type;
            const char *suffix = code_gen_type_suffix(elem_type);
            indented_fprintf(gen, 1, "_return_value = rt_array_clone_%s(__caller_arena__, _return_value);\n", suffix);
        }
        else if (kind == TYPE_STRUCT)
        {
            // Promote struct to caller's arena - need size of the struct
            // _return_value is a struct value (not pointer), so we take its address,
            // promote to caller's arena, and dereference the result
            const char *struct_name = stmt->return_type->as.struct_type.name;
            indented_fprintf(gen, 1, "_return_value = *(%s *)rt_arena_promote(__caller_arena__, &_return_value, sizeof(%s));\n", struct_name, struct_name);
        }
        else if (kind == TYPE_FUNCTION)
        {
            // Closures - promote the closure struct using its actual size
            indented_fprintf(gen, 1, "_return_value = rt_arena_promote(__caller_arena__, _return_value, _return_value->size);\n");
        }
        else if (kind == TYPE_ANY)
        {
            // Any values may contain heap-allocated data (strings, arrays).
            // Use runtime promotion to copy to caller's arena.
            indented_fprintf(gen, 1, "_return_value = rt_any_promote(__caller_arena__, _return_value);\n");
        }
    }

    // Destroy local arena for main and non-shared functions
    // (shared functions just alias the caller's arena, so don't destroy)
    if (is_main || !is_shared)
    {
        indented_fprintf(gen, 1, "rt_arena_destroy(__local_arena__);\n");
    }

    // Return _return_value only if needed; otherwise, plain return.
    if (has_return_value)
    {
        indented_fprintf(gen, 1, "return _return_value;\n");
    }
    else
    {
        indented_fprintf(gen, 1, "return;\n");
    }
    indented_fprintf(gen, 0, "}\n\n");

    // Exit arena scope in symbol table (all functions have arena context now)
    symbol_table_exit_arena(gen->symbol_table);

    symbol_table_pop_scope(gen->symbol_table);

    // Clear captured primitives list
    code_gen_clear_captured_primitives(gen);

    gen->current_function = old_function;
    gen->current_return_type = old_return_type;
    gen->current_func_modifier = old_func_modifier;
    gen->in_private_context = old_in_private_context;
    gen->in_shared_context = old_in_shared_context;
    gen->current_arena_var = old_arena_var;
    gen->arena_depth = old_arena_depth;
}

void code_gen_return_statement(CodeGen *gen, ReturnStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_return_statement");
    /* Check if returning from a void function/lambda */
    int is_void_return = (gen->current_return_type && gen->current_return_type->kind == TYPE_VOID);

    /* Check if this return contains a tail call that should be optimized */
    if (gen->in_tail_call_function && stmt->value &&
        stmt->value->type == EXPR_CALL && stmt->value->as.call.is_tail_call)
    {
        CallExpr *call = &stmt->value->as.call;
        FunctionStmt *fn = gen->tail_call_fn;

        /* Generate parameter assignments */
        /* For multiple parameters, we need temp variables to handle cases like
           return f(b, a) when the current params are (a, b) */
        if (fn->param_count > 1)
        {
            /* First, generate temp variables for all new argument values */
            for (int i = 0; i < call->arg_count; i++)
            {
                const char *param_type_c = get_c_type(gen->arena, fn->params[i].type);
                char *arg_str = code_gen_expression(gen, call->arguments[i]);
                indented_fprintf(gen, indent, "%s __tail_arg_%d__ = %s;\n",
                                 param_type_c, i, arg_str);
            }
            /* Then, assign temps to actual parameters */
            for (int i = 0; i < call->arg_count; i++)
            {
                char *param_name = get_var_name(gen->arena, fn->params[i].name);
                indented_fprintf(gen, indent, "%s = __tail_arg_%d__;\n",
                                 param_name, i);
            }
        }
        else if (fn->param_count == 1)
        {
            /* Single parameter - direct assignment is safe */
            char *param_name = get_var_name(gen->arena, fn->params[0].name);
            char *arg_str = code_gen_expression(gen, call->arguments[0]);
            indented_fprintf(gen, indent, "%s = %s;\n", param_name, arg_str);
        }
        /* Continue the tail call loop */
        indented_fprintf(gen, indent, "continue;\n");
        return;
    }

    /* Normal return */
    if (stmt->value && !is_void_return)
    {
        /* If returning a lambda expression directly, allocate it in the caller's
         * arena so captured variables survive the function's arena destruction. */
        bool is_lambda_return = (stmt->value->type == EXPR_LAMBDA);
        if (is_lambda_return)
        {
            gen->allocate_closure_in_caller_arena = true;
        }

        char *value_str = code_gen_expression(gen, stmt->value);

        if (is_lambda_return)
        {
            gen->allocate_closure_in_caller_arena = false;
        }

        /* Handle boxing when function returns 'any' but expression is a concrete type */
        if (gen->current_return_type != NULL && gen->current_return_type->kind == TYPE_ANY &&
            stmt->value->expr_type != NULL && stmt->value->expr_type->kind != TYPE_ANY)
        {
            value_str = code_gen_box_value(gen, value_str, stmt->value->expr_type);
        }

        indented_fprintf(gen, indent, "_return_value = %s;\n", value_str);
    }

    /* Clean up all active loop arenas before returning (innermost first) */
    for (int i = gen->loop_arena_depth - 1; i >= 0; i--)
    {
        if (gen->loop_arena_stack[i] != NULL)
        {
            indented_fprintf(gen, indent, "rt_arena_destroy(%s);\n", gen->loop_arena_stack[i]);
        }
    }

    /* Clean up all active private block arenas before returning (innermost first).
     * The function-level arena is NOT on this stack - it's destroyed at the return label.
     * This stack only contains private block arenas that need explicit cleanup. */
    for (int i = gen->arena_stack_depth - 1; i >= 0; i--)
    {
        if (gen->arena_stack[i] != NULL)
        {
            indented_fprintf(gen, indent, "rt_arena_destroy(%s);\n", gen->arena_stack[i]);
        }
    }

    indented_fprintf(gen, indent, "goto %s_return;\n", gen->current_function);
}

void code_gen_if_statement(CodeGen *gen, IfStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_if_statement");
    char *cond_str = code_gen_expression(gen, stmt->condition);
    indented_fprintf(gen, indent, "if (%s) {\n", cond_str);
    code_gen_statement(gen, stmt->then_branch, indent + 1);
    indented_fprintf(gen, indent, "}\n");
    if (stmt->else_branch)
    {
        indented_fprintf(gen, indent, "else {\n");
        code_gen_statement(gen, stmt->else_branch, indent + 1);
        indented_fprintf(gen, indent, "}\n");
    }
}

void code_gen_statement(CodeGen *gen, Stmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_statement");
    switch (stmt->type)
    {
    case STMT_EXPR:
        code_gen_expression_statement(gen, &stmt->as.expression, indent);
        break;
    case STMT_VAR_DECL:
        code_gen_var_declaration(gen, &stmt->as.var_decl, indent);
        break;
    case STMT_FUNCTION:
        code_gen_function(gen, &stmt->as.function);
        break;
    case STMT_RETURN:
        code_gen_return_statement(gen, &stmt->as.return_stmt, indent);
        break;
    case STMT_BLOCK:
        code_gen_block(gen, &stmt->as.block, indent);
        break;
    case STMT_IF:
        code_gen_if_statement(gen, &stmt->as.if_stmt, indent);
        break;
    case STMT_WHILE:
        code_gen_while_statement(gen, &stmt->as.while_stmt, indent);
        break;
    case STMT_FOR:
        code_gen_for_statement(gen, &stmt->as.for_stmt, indent);
        break;
    case STMT_FOR_EACH:
        code_gen_for_each_statement(gen, &stmt->as.for_each_stmt, indent);
        break;
    case STMT_BREAK:
        // If in a loop with per-iteration arena, destroy it before breaking
        if (gen->loop_arena_var)
        {
            indented_fprintf(gen, indent, "{ rt_arena_destroy(%s); break; }\n", gen->loop_arena_var);
        }
        else
        {
            indented_fprintf(gen, indent, "break;\n");
        }
        break;
    case STMT_CONTINUE:
        // If there's a loop cleanup label (for per-iteration arena), jump to it
        // The cleanup label destroys the arena and falls through to continue/increment
        if (gen->loop_cleanup_label)
        {
            indented_fprintf(gen, indent, "goto %s;\n", gen->loop_cleanup_label);
        }
        // In for loops without arena, continue needs to jump to the continue label (before increment)
        else if (gen->for_continue_label)
        {
            indented_fprintf(gen, indent, "goto %s;\n", gen->for_continue_label);
        }
        // In while/for-each loops without arena, regular continue works fine
        else
        {
            indented_fprintf(gen, indent, "continue;\n");
        }
        break;
    case STMT_IMPORT:
        /* For namespaced imports, emit the imported module's function definitions.
         * Non-namespaced imports have their statements merged by the parser,
         * so they don't need special handling here.
         *
         * If the module was ALSO imported directly (without namespace), the functions
         * are already in the main module and we should NOT emit them again to avoid
         * duplicate definition errors. */
        if (stmt->as.import.namespace != NULL && stmt->as.import.imported_stmts != NULL &&
            !stmt->as.import.also_imported_directly)
        {
            for (int i = 0; i < stmt->as.import.imported_count; i++)
            {
                code_gen_statement(gen, stmt->as.import.imported_stmts[i], indent);
            }
        }
        break;
    case STMT_PRAGMA:
        /* Pragmas are collected in code_gen_module and emitted at the top of the file.
         * No action needed here during statement code generation. */
        break;
    case STMT_TYPE_DECL:
        /* Type declarations are handled at the module level where forward declarations
         * are emitted. No code generation is needed for the statement itself. */
        break;
    case STMT_STRUCT_DECL:
        /* Struct declarations are handled at the module level where typedef
         * declarations are emitted. No code generation is needed for the statement itself. */
        break;

    case STMT_LOCK:
    {
        /* Lock block: lock(sync_var) => { ... }
         * Generates:
         *   rt_sync_lock(&sync_var);
         *   { body }
         *   rt_sync_unlock(&sync_var);
         */
        LockStmt *lock_stmt = &stmt->as.lock_stmt;

        /* Generate the lock expression (should be a variable name) */
        char *lock_var = code_gen_expression(gen, lock_stmt->lock_expr);

        /* Generate lock acquisition */
        indented_fprintf(gen, indent, "rt_sync_lock(&%s);\n", lock_var);

        /* Generate lock body in a block */
        indented_fprintf(gen, indent, "{\n");
        code_gen_statement(gen, lock_stmt->body, indent + 1);
        indented_fprintf(gen, indent, "}\n");

        /* Generate lock release */
        indented_fprintf(gen, indent, "rt_sync_unlock(&%s);\n", lock_var);
        break;
    }
    }
}
