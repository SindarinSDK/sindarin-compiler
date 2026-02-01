/*
 * code_gen_stmt_core.c - Core statement code generation
 *
 * Contains the main statement dispatcher and small helper functions.
 * Large functions have been split into separate files:
 *   - code_gen_stmt_import.c   - Import helpers
 *   - code_gen_stmt_thread.c   - Thread sync statement
 *   - code_gen_stmt_struct.c   - Struct method generation
 *   - code_gen_stmt_var.c      - Variable declaration
 *   - code_gen_stmt_func.c     - Function generation
 *   - code_gen_stmt_return.c   - Return statement
 */

#include "code_gen/stmt/code_gen_stmt.h"
#include "code_gen/stmt/code_gen_stmt_loop.h"
#include "code_gen/stmt/code_gen_stmt_capture.h"
#include "code_gen/stmt/code_gen_stmt_import.h"
#include "code_gen/stmt/code_gen_stmt_thread.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include "symbol_table/symbol_table_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void code_gen_expression_statement(CodeGen *gen, ExprStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_expression_statement");

    /* Special handling for thread sync statements */
    if (stmt->expression->type == EXPR_THREAD_SYNC)
    {
        code_gen_thread_sync_statement(gen, stmt->expression, indent);
        return;
    }

    char *expr_str = code_gen_expression(gen, stmt->expression);
    DEBUG_VERBOSE("Expression statement type: %p", (void*)stmt->expression->expr_type);

    if (stmt->expression->expr_type != NULL &&
        stmt->expression->expr_type->kind == TYPE_STRING && expression_produces_temp(stmt->expression))
    {
        /* Skip freeing in arena context - arena handles cleanup */
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
        indented_fprintf(gen, indent, "%s;\n", expr_str);
    }
    else
    {
        indented_fprintf(gen, indent, "%s;\n", expr_str);
    }
}

void code_gen_free_locals(CodeGen *gen, Scope *scope, bool is_function, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_free_locals");

    /* Skip manual freeing when in arena context */
    if (gen->current_arena_var != NULL)
    {
        return;
    }

    Symbol *sym = scope->symbols;
    while (sym)
    {
        if (sym->type && sym->type->kind == TYPE_STRING && sym->kind == SYMBOL_LOCAL)
        {
            char *var_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, sym->name));
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
            char *var_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, sym->name));
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

    symbol_table_push_scope(gen->symbol_table);

    indented_fprintf(gen, indent, "{\n");

    for (int i = 0; i < stmt->count; i++)
    {
        code_gen_statement(gen, stmt->statements[i], indent + 1);
    }
    code_gen_free_locals(gen, gen->symbol_table->current, false, indent + 1);

    indented_fprintf(gen, indent, "}\n");
    symbol_table_pop_scope(gen->symbol_table);
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

    /* Emit any attached comments */
    if (stmt->comment_count > 0 && stmt->comments != NULL)
    {
        for (int i = 0; i < stmt->comment_count; i++)
        {
            if (stmt->comments[i] != NULL)
            {
                indented_fprintf(gen, indent, "//%s\n", stmt->comments[i]);
            }
        }
    }

    gen->current_indent = indent;
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
        indented_fprintf(gen, indent, "break;\n");
        break;
    case STMT_CONTINUE:
        if (gen->for_continue_label)
        {
            indented_fprintf(gen, indent, "goto %s;\n", gen->for_continue_label);
        }
        else
        {
            indented_fprintf(gen, indent, "continue;\n");
        }
        break;
    case STMT_IMPORT:
        /* Handle namespaced imports */
        if (stmt->as.import.namespace != NULL && stmt->as.import.imported_stmts != NULL)
        {
            bool emit_functions = true;
            Token ns_name = *stmt->as.import.namespace;
            Symbol *ns_symbol = NULL;

            if (gen->current_namespace_prefix != NULL)
            {
                Token parent_ns_token;
                parent_ns_token.start = gen->current_namespace_prefix;
                parent_ns_token.length = strlen(gen->current_namespace_prefix);
                parent_ns_token.type = TOKEN_IDENTIFIER;
                parent_ns_token.line = 0;
                parent_ns_token.filename = NULL;
                ns_symbol = symbol_table_lookup_nested_namespace(gen->symbol_table, parent_ns_token, ns_name);
            }

            if (ns_symbol == NULL)
            {
                ns_symbol = symbol_table_lookup_symbol(gen->symbol_table, ns_name);
            }

            if (ns_symbol != NULL && ns_symbol->is_namespace)
            {
                symbol_table_push_scope(gen->symbol_table);
                add_namespace_symbols_to_scope(gen, ns_symbol);
            }

            const char *old_namespace_prefix = gen->current_namespace_prefix;
            const char *old_canonical_module = gen->current_canonical_module;
            char ns_prefix[256];
            int ns_len = ns_name.length < 255 ? ns_name.length : 255;
            memcpy(ns_prefix, ns_name.start, ns_len);
            ns_prefix[ns_len] = '\0';
            gen->current_namespace_prefix = arena_strdup(gen->arena, ns_prefix);

            if (ns_symbol != NULL && ns_symbol->canonical_module_name != NULL)
            {
                gen->current_canonical_module = ns_symbol->canonical_module_name;
            }
            else
            {
                char mod_path[512];
                int mod_len = stmt->as.import.module_name.length < 511 ? stmt->as.import.module_name.length : 511;
                memcpy(mod_path, stmt->as.import.module_name.start, mod_len);
                mod_path[mod_len] = '\0';

                const char *base_name = mod_path;
                for (int k = mod_len - 1; k >= 0; k--)
                {
                    if (mod_path[k] == '/' || mod_path[k] == '\\')
                    {
                        base_name = &mod_path[k + 1];
                        break;
                    }
                }
                char *canonical = arena_strdup(gen->arena, base_name);
                int can_len = strlen(canonical);
                if (can_len > 3 && strcmp(canonical + can_len - 3, ".sn") == 0)
                {
                    canonical[can_len - 3] = '\0';
                }
                gen->current_canonical_module = canonical;
            }

            if (emit_functions)
            {
                emit_import_forward_declarations_recursive(gen, stmt->as.import.imported_stmts,
                                                           stmt->as.import.imported_count,
                                                           gen->current_namespace_prefix);
            }

            for (int i = 0; i < stmt->as.import.imported_count; i++)
            {
                Stmt *imported_stmt = stmt->as.import.imported_stmts[i];
                if (imported_stmt == NULL)
                    continue;
                if (!emit_functions && imported_stmt->type == STMT_FUNCTION)
                    continue;
                code_gen_statement(gen, imported_stmt, indent);
            }

            if (emit_functions)
            {
                for (int i = 0; i < stmt->as.import.imported_count; i++)
                {
                    Stmt *imported_stmt = stmt->as.import.imported_stmts[i];
                    if (imported_stmt != NULL && imported_stmt->type == STMT_STRUCT_DECL)
                    {
                        code_gen_struct_methods(gen, &imported_stmt->as.struct_decl, indent);
                    }
                }
            }

            gen->current_namespace_prefix = old_namespace_prefix;
            gen->current_canonical_module = old_canonical_module;

            if (ns_symbol != NULL && ns_symbol->is_namespace)
            {
                symbol_table_pop_scope(gen->symbol_table);
            }
        }
        break;
    case STMT_PRAGMA:
        /* Pragmas handled at module level */
        break;
    case STMT_TYPE_DECL:
        /* Type declarations handled at module level */
        break;
    case STMT_STRUCT_DECL:
        /* Struct declarations handled at module level */
        break;
    case STMT_LOCK:
    {
        LockStmt *lock_stmt = &stmt->as.lock_stmt;
        char *lock_var = code_gen_expression(gen, lock_stmt->lock_expr);
        indented_fprintf(gen, indent, "rt_sync_lock(&%s);\n", lock_var);
        indented_fprintf(gen, indent, "{\n");
        code_gen_statement(gen, lock_stmt->body, indent + 1);
        indented_fprintf(gen, indent, "}\n");
        indented_fprintf(gen, indent, "rt_sync_unlock(&%s);\n", lock_var);
        break;
    }
    }
}
