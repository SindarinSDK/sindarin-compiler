#include "type_checker/stmt/type_checker_stmt_func.h"
#include "type_checker/stmt/type_checker_stmt.h"
#include "type_checker/util/type_checker_util.h"
#include "type_checker/expr/type_checker_expr.h"
#include "symbol_table/symbol_table_core.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

void add_arena_builtin(SymbolTable *table, Token *ref_token)
{
    Token arena_token;
    arena_token.start = "arena";
    arena_token.length = 5;
    arena_token.line = ref_token->line;
    arena_token.filename = ref_token->filename;
    arena_token.type = TOKEN_IDENTIFIER;

    /* Type is *void (pointer to void) */
    Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
    Type *arena_type = ast_create_pointer_type(table->arena, void_type);
    symbol_table_add_symbol(table, arena_token, arena_type);
}

void type_check_function_body_only(Stmt *stmt, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking function body only: %.*s",
                  stmt->as.function.name.length, stmt->as.function.name.start);

    /* Skip if already type-checked (prevents re-type-checking in diamond imports) */
    if (stmt->as.function.body_type_checked)
    {
        DEBUG_VERBOSE("Skipping already type-checked function body: %.*s",
                      stmt->as.function.name.length, stmt->as.function.name.start);
        return;
    }

    Arena *arena = table->arena;

    symbol_table_push_scope(table);

    /* For non-native functions, add 'arena' as a built-in identifier */
    if (!stmt->as.function.is_native && stmt->as.function.body_count > 0)
    {
        add_arena_builtin(table, &stmt->as.function.name);
    }

    for (int i = 0; i < stmt->as.function.param_count; i++)
    {
        Parameter param = stmt->as.function.params[i];
        if (param.type == NULL)
        {
            param.type = ast_create_primitive_type(arena, TYPE_NIL);
        }
        symbol_table_add_symbol_full(table, param.name, param.type, SYMBOL_PARAM, param.mem_qualifier);

        /* Set sync modifier if present */
        if (param.sync_modifier == SYNC_ATOMIC)
        {
            Symbol *sym = symbol_table_lookup_symbol_current(table, param.name);
            if (sym != NULL)
            {
                sym->sync_mod = SYNC_ATOMIC;
            }
        }
    }

    table->current->next_local_offset = table->current->next_param_offset;

    /* Track native function context for pointer type restrictions */
    if (stmt->as.function.is_native)
    {
        native_context_enter();
    }

    for (int i = 0; i < stmt->as.function.body_count; i++)
    {
        type_check_stmt(stmt->as.function.body[i], table, stmt->as.function.return_type);
    }

    if (stmt->as.function.is_native)
    {
        native_context_exit();
    }

    symbol_table_pop_scope(table);

    /* Mark as type-checked to prevent re-type-checking in diamond imports */
    stmt->as.function.body_type_checked = true;
}

void type_check_function(Stmt *stmt, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking function with %d parameters", stmt->as.function.param_count);

    /* Enforce that pointer types in function signatures require native keyword */
    if (!stmt->as.function.is_native)
    {
        if (stmt->as.function.return_type &&
            stmt->as.function.return_type->kind == TYPE_POINTER)
        {
            type_error(&stmt->as.function.name,
                       "Pointer return type requires 'native' function");
            return;
        }

        for (int i = 0; i < stmt->as.function.param_count; i++)
        {
            Type *param_type = stmt->as.function.params[i].type;
            if (param_type && param_type->kind == TYPE_POINTER)
            {
                type_error(&stmt->as.function.params[i].name,
                           "Pointer parameter type requires 'native' function");
                return;
            }
        }
    }

    /* Special validation for main function parameters */
    bool is_main_func = (stmt->as.function.name.length == 4 &&
                         strncmp(stmt->as.function.name.start, "main", 4) == 0);

    if (is_main_func && stmt->as.function.param_count > 0)
    {
        if (stmt->as.function.param_count != 1)
        {
            type_error(&stmt->as.function.name,
                       "main function can only have one parameter: str[]");
            return;
        }

        Type *param_type = stmt->as.function.params[0].type;
        bool is_string_array = (param_type != NULL &&
                               param_type->kind == TYPE_ARRAY &&
                               param_type->as.array.element_type != NULL &&
                               param_type->as.array.element_type->kind == TYPE_STRING);

        if (!is_string_array)
        {
            type_error(&stmt->as.function.params[0].name,
                       "main function parameter must be of type str[]");
            return;
        }
    }

    /* Create function type from declaration */
    Arena *arena = table->arena;
    Type **param_types = (Type **)arena_alloc(arena, sizeof(Type *) * stmt->as.function.param_count);
    for (int i = 0; i < stmt->as.function.param_count; i++) {
        Type *param_type = stmt->as.function.params[i].type;
        if (param_type == NULL) {
            param_type = ast_create_primitive_type(arena, TYPE_NIL);
        }
        param_types[i] = param_type;
    }
    Type *func_type = ast_create_function_type(arena, stmt->as.function.return_type,
                                                param_types, stmt->as.function.param_count);
    func_type->as.function.is_variadic = stmt->as.function.is_variadic;
    func_type->as.function.is_native = stmt->as.function.is_native;
    func_type->as.function.has_body = (stmt->as.function.body_count > 0);
    func_type->as.function.has_arena_param = stmt->as.function.has_arena_param;

    DEBUG_VERBOSE("Type checking function '%.*s': is_native=%d, has_arena_param=%d",
                  stmt->as.function.name.length, stmt->as.function.name.start,
                  stmt->as.function.is_native, func_type->as.function.has_arena_param);

    /* Store parameter memory qualifiers in the function type */
    if (stmt->as.function.param_count > 0)
    {
        bool has_non_default_qual = false;
        for (int i = 0; i < stmt->as.function.param_count; i++)
        {
            if (stmt->as.function.params[i].mem_qualifier != MEM_DEFAULT)
            {
                has_non_default_qual = true;
                break;
            }
        }

        if (has_non_default_qual)
        {
            func_type->as.function.param_mem_quals = (MemoryQualifier *)arena_alloc(arena,
                sizeof(MemoryQualifier) * stmt->as.function.param_count);
            for (int i = 0; i < stmt->as.function.param_count; i++)
            {
                func_type->as.function.param_mem_quals[i] = stmt->as.function.params[i].mem_qualifier;
            }
        }
    }

    /* Validate private function return type */
    FunctionModifier modifier = stmt->as.function.modifier;
    if (modifier == FUNC_PRIVATE)
    {
        Type *return_type = stmt->as.function.return_type;
        if (!can_escape_private(return_type))
        {
            const char *reason = get_private_escape_block_reason(return_type);
            char error_msg[512];
            if (reason != NULL)
            {
                snprintf(error_msg, sizeof(error_msg),
                         "Private function cannot return this type: %s", reason);
            }
            else
            {
                snprintf(error_msg, sizeof(error_msg),
                         "Private function can only return primitive types or structs with only primitive fields");
            }
            type_error(&stmt->as.function.name, error_msg);
        }
    }

    FunctionModifier effective_modifier = modifier;

    /* Check for duplicate function definition */
    Symbol *existing = symbol_table_lookup_symbol(table, stmt->as.function.name);
    if (existing != NULL && existing->is_function)
    {
        char name_str[128];
        int name_len = stmt->as.function.name.length < 127 ? stmt->as.function.name.length : 127;
        memcpy(name_str, stmt->as.function.name.start, name_len);
        name_str[name_len] = '\0';

        char msg[256];
        snprintf(msg, sizeof(msg), "Function '%s' is already defined (possible import collision)", name_str);
        type_error(&stmt->as.function.name, msg);
        return;
    }

    /* Add function symbol to current scope */
    if (stmt->as.function.is_native)
    {
        symbol_table_add_native_function(table, stmt->as.function.name, func_type, effective_modifier, modifier);
    }
    else
    {
        symbol_table_add_function(table, stmt->as.function.name, func_type, effective_modifier, modifier);
    }

    /* Set c_alias on the symbol if the function has one */
    if (stmt->as.function.c_alias != NULL)
    {
        Symbol *func_sym = symbol_table_lookup_symbol_current(table, stmt->as.function.name);
        if (func_sym != NULL)
        {
            func_sym->c_alias = stmt->as.function.c_alias;
        }
    }

    symbol_table_push_scope(table);

    /* For non-native functions, add 'arena' as a built-in identifier */
    if (!stmt->as.function.is_native && stmt->as.function.body_count > 0)
    {
        add_arena_builtin(table, &stmt->as.function.name);
    }

    for (int i = 0; i < stmt->as.function.param_count; i++)
    {
        Parameter param = stmt->as.function.params[i];
        DEBUG_VERBOSE("Adding parameter %d: %.*s", i, param.name.length, param.name.start);

        if (param.type == NULL)
        {
            type_error(&param.name, "Parameter type is missing");
            param.type = ast_create_primitive_type(arena, TYPE_NIL);
        }

        /* Validate parameter memory qualifier */
        if (param.mem_qualifier == MEM_AS_VAL)
        {
            if (is_primitive_type(param.type))
            {
                DEBUG_VERBOSE("Warning: 'as val' on primitive parameter has no effect");
            }
        }
        else if (param.mem_qualifier == MEM_AS_REF)
        {
            if (!is_primitive_type(param.type) && param.type->kind != TYPE_STRUCT)
            {
                type_error(&param.name, "'as ref' only applies to primitive or struct parameters");
            }
        }

        symbol_table_add_symbol_full(table, param.name, param.type, SYMBOL_PARAM, param.mem_qualifier);

        /* Set sync modifier if present */
        if (param.sync_modifier == SYNC_ATOMIC)
        {
            Symbol *sym = symbol_table_lookup_symbol_current(table, param.name);
            if (sym != NULL)
            {
                sym->sync_mod = SYNC_ATOMIC;
            }
        }
    }

    table->current->next_local_offset = table->current->next_param_offset;

    /* Track native function context for pointer type restrictions */
    if (stmt->as.function.is_native)
    {
        native_context_enter();
    }

    for (int i = 0; i < stmt->as.function.body_count; i++)
    {
        type_check_stmt(stmt->as.function.body[i], table, stmt->as.function.return_type);
    }

    if (stmt->as.function.is_native)
    {
        native_context_exit();
    }

    symbol_table_pop_scope(table);

    /* Mark as type-checked to prevent re-type-checking in diamond imports */
    stmt->as.function.body_type_checked = true;
}
