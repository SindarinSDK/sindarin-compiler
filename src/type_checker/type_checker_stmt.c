#include "type_checker/type_checker_stmt.h"
#include "type_checker/type_checker_util.h"
#include "type_checker/type_checker_expr.h"
#include "symbol_table/symbol_table_core.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

/* Reserved keyword table for namespace validation */
static const char *reserved_keywords[] = {
    "fn", "var", "return", "if", "else", "for", "while", "break", "continue",
    "in", "import", "nil", "int", "long", "double", "char", "str", "bool",
    "byte", "void", "shared", "private", "as", "val", "ref", "true", "false",
    "native",
    NULL
};

/* Check if a token matches a reserved keyword.
 * Returns the keyword string if it matches, NULL otherwise. */
static const char *is_reserved_keyword(Token token)
{
    for (int i = 0; reserved_keywords[i] != NULL; i++)
    {
        const char *keyword = reserved_keywords[i];
        int keyword_len = strlen(keyword);
        if (token.length == keyword_len &&
            memcmp(token.start, keyword, keyword_len) == 0)
        {
            return keyword;
        }
    }
    return NULL;
}

/* Infer missing lambda types from a function type annotation */
static void infer_lambda_types(Expr *lambda_expr, Type *func_type)
{
    if (lambda_expr == NULL || lambda_expr->type != EXPR_LAMBDA)
        return;
    if (func_type == NULL || func_type->kind != TYPE_FUNCTION)
        return;

    LambdaExpr *lambda = &lambda_expr->as.lambda;

    /* Infer is_native from the function type FIRST, before checking parameter count.
     * This ensures the lambda is marked as native even if signatures don't match,
     * which enables better error messages for native lambda signature mismatches. */
    if (func_type->as.function.is_native && !lambda->is_native)
    {
        lambda->is_native = true;
        DEBUG_VERBOSE("Inferred is_native from function type");
    }

    /* Check parameter count matches */
    if (lambda->param_count != func_type->as.function.param_count)
    {
        DEBUG_VERBOSE("Lambda param count %d doesn't match function type param count %d",
                      lambda->param_count, func_type->as.function.param_count);
        return;
    }

    /* Infer missing parameter types */
    for (int i = 0; i < lambda->param_count; i++)
    {
        if (lambda->params[i].type == NULL)
        {
            lambda->params[i].type = func_type->as.function.param_types[i];
            DEBUG_VERBOSE("Inferred parameter %d type from function type", i);
        }
    }

    /* Infer missing return type */
    if (lambda->return_type == NULL)
    {
        lambda->return_type = func_type->as.function.return_type;
        DEBUG_VERBOSE("Inferred return type from function type");
    }
}

static void type_check_var_decl(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    (void)return_type;
    DEBUG_VERBOSE("Type checking variable declaration: %.*s", stmt->as.var_decl.name.length, stmt->as.var_decl.name.start);

    // Check for redeclaration in the current scope
    Symbol *existing = symbol_table_lookup_symbol_current(table, stmt->as.var_decl.name);
    if (existing != NULL)
    {
        type_error(&stmt->as.var_decl.name, "Variable is already declared in this scope");
        return;
    }

    Type *decl_type = stmt->as.var_decl.type;
    Type *init_type = NULL;
    bool added_for_recursion = false;
    if (stmt->as.var_decl.initializer)
    {
        /* If initializer is a lambda with missing types, infer from declared type */
        if (stmt->as.var_decl.initializer->type == EXPR_LAMBDA &&
            decl_type != NULL && decl_type->kind == TYPE_FUNCTION)
        {
            infer_lambda_types(stmt->as.var_decl.initializer, decl_type);

            /* For recursive lambdas: Add the variable to scope BEFORE type-checking
             * the lambda body, so the lambda can reference itself. Mark it as a
             * function so it can be called recursively. */
            symbol_table_add_symbol_with_kind(table, stmt->as.var_decl.name, decl_type, SYMBOL_LOCAL);
            Symbol *sym = symbol_table_lookup_symbol_current(table, stmt->as.var_decl.name);
            if (sym != NULL)
            {
                sym->is_function = true;  /* Allow calling it like a function */
            }
            added_for_recursion = true;
        }

        init_type = type_check_expr(stmt->as.var_decl.initializer, table);
        if (init_type == NULL)
        {
            // If we have a declared type, use it; otherwise use NIL as placeholder
            Type *fallback = decl_type ? decl_type : ast_create_primitive_type(table->arena, TYPE_NIL);
            symbol_table_add_symbol_with_kind(table, stmt->as.var_decl.name, fallback, SYMBOL_LOCAL);
            return;
        }
        // Void thread spawns cannot be assigned to variables (fire-and-forget only)
        if (stmt->as.var_decl.initializer->type == EXPR_THREAD_SPAWN &&
            init_type->kind == TYPE_VOID)
        {
            type_error(&stmt->as.var_decl.name, "Cannot assign void thread spawn to variable");
            return;
        }
        // For empty array literals, adopt the declared type for code generation
        if (decl_type && init_type->kind == TYPE_ARRAY &&
            init_type->as.array.element_type->kind == TYPE_NIL &&
            decl_type->kind == TYPE_ARRAY)
        {
            // Update the expression's type to match the declared type
            stmt->as.var_decl.initializer->expr_type = decl_type;
            init_type = decl_type;
        }
        // For int[] assigned to byte[], update the expression type to byte[]
        // This allows int literals to be used in byte array literals
        if (decl_type && decl_type->kind == TYPE_ARRAY &&
            decl_type->as.array.element_type->kind == TYPE_BYTE &&
            init_type->kind == TYPE_ARRAY &&
            init_type->as.array.element_type->kind == TYPE_INT)
        {
            stmt->as.var_decl.initializer->expr_type = decl_type;
            init_type = decl_type;
        }
        // For int[] assigned to int32[], uint32[], uint[], or float[], update the expression type
        // This allows int literals to be used in C interop type array literals
        if (decl_type && decl_type->kind == TYPE_ARRAY &&
            init_type->kind == TYPE_ARRAY &&
            init_type->as.array.element_type->kind == TYPE_INT)
        {
            TypeKind decl_elem = decl_type->as.array.element_type->kind;
            if (decl_elem == TYPE_INT32 || decl_elem == TYPE_UINT32 ||
                decl_elem == TYPE_UINT || decl_elem == TYPE_FLOAT)
            {
                stmt->as.var_decl.initializer->expr_type = decl_type;
                init_type = decl_type;
            }
        }
        // For double[] assigned to float[], update the expression type
        if (decl_type && decl_type->kind == TYPE_ARRAY &&
            decl_type->as.array.element_type->kind == TYPE_FLOAT &&
            init_type->kind == TYPE_ARRAY &&
            init_type->as.array.element_type->kind == TYPE_DOUBLE)
        {
            stmt->as.var_decl.initializer->expr_type = decl_type;
            init_type = decl_type;
        }
        // For 2D arrays: byte[][] = {{1,2,3},...} - coerce inner arrays from int[] to target element type
        // Also handles int32[][], uint32[][], uint[][], float[][]
        if (decl_type && decl_type->kind == TYPE_ARRAY &&
            decl_type->as.array.element_type->kind == TYPE_ARRAY &&
            init_type->kind == TYPE_ARRAY &&
            init_type->as.array.element_type->kind == TYPE_ARRAY)
        {
            Type *decl_inner = decl_type->as.array.element_type;
            Type *init_inner = init_type->as.array.element_type;
            TypeKind decl_elem = decl_inner->as.array.element_type->kind;
            TypeKind init_elem = init_inner->as.array.element_type->kind;
            // If inner types differ but are compatible (int -> byte/int32/uint32/uint/float)
            bool needs_coercion = init_elem == TYPE_INT &&
                (decl_elem == TYPE_BYTE || decl_elem == TYPE_INT32 ||
                 decl_elem == TYPE_UINT32 || decl_elem == TYPE_UINT ||
                 decl_elem == TYPE_FLOAT);
            // Also handle double -> float coercion for 2D arrays
            if (!needs_coercion && init_elem == TYPE_DOUBLE && decl_elem == TYPE_FLOAT)
            {
                needs_coercion = true;
            }
            if (needs_coercion)
            {
                // Update outer array type
                stmt->as.var_decl.initializer->expr_type = decl_type;
                // Update each inner array element's type
                Expr *init = stmt->as.var_decl.initializer;
                if (init->type == EXPR_ARRAY)
                {
                    for (int i = 0; i < init->as.array.element_count; i++)
                    {
                        Expr *elem = init->as.array.elements[i];
                        if (elem->type == EXPR_ARRAY)
                        {
                            elem->expr_type = decl_inner;
                        }
                    }
                }
                init_type = decl_type;
            }
        }
    }

    // Type inference: if no declared type, infer from initializer
    if (decl_type == NULL)
    {
        if (init_type == NULL)
        {
            type_error(&stmt->as.var_decl.name, "Cannot infer type without initializer");
            decl_type = ast_create_primitive_type(table->arena, TYPE_NIL);
        }
        else
        {
            decl_type = init_type;
            // Update the statement's type for code generation
            stmt->as.var_decl.type = decl_type;
        }
    }

    // Reject pointer variable declarations in non-native functions.
    // Pointer types can only be stored in variables within native functions.
    // Regular functions must use 'as val' to unwrap pointer returns immediately.
    if (decl_type && decl_type->kind == TYPE_POINTER && !native_context_is_active())
    {
        type_error(&stmt->as.var_decl.name,
                   "Pointer variables can only be declared in native functions");
    }

    // Reject pointer return values from native functions in non-native context.
    // If a native function returns a pointer, it must be unwrapped with 'as val'.
    if (init_type && init_type->kind == TYPE_POINTER && !native_context_is_active())
    {
        type_error(&stmt->as.var_decl.name,
                   "Pointer types not allowed in non-native functions, use 'as val'");
    }

    // Validate memory qualifier usage
    MemoryQualifier mem_qual = stmt->as.var_decl.mem_qualifier;
    if (mem_qual == MEM_AS_REF)
    {
        // 'as ref' can only be used with primitive types
        if (!is_primitive_type(decl_type))
        {
            type_error(&stmt->as.var_decl.name, "'as ref' can only be used with primitive types");
        }
    }
    else if (mem_qual == MEM_AS_VAL)
    {
        // 'as val' is meaningful only for reference types (arrays, strings)
        // For primitives, it's a no-op but we allow it
        if (is_primitive_type(decl_type))
        {
            DEBUG_VERBOSE("Warning: 'as val' on primitive type has no effect");
        }
    }

    /* Only add symbol if we didn't already add it for recursive lambda support */
    if (!added_for_recursion)
    {
        symbol_table_add_symbol_with_kind(table, stmt->as.var_decl.name, decl_type, SYMBOL_LOCAL);
    }

    /* Handle sync modifier - set on symbol and validate type */
    if (stmt->as.var_decl.sync_modifier == SYNC_ATOMIC)
    {
        /* Validate sync is only on atomic-compatible types (integers including byte/char) */
        if (decl_type->kind != TYPE_INT && decl_type->kind != TYPE_LONG &&
            decl_type->kind != TYPE_INT32 && decl_type->kind != TYPE_UINT &&
            decl_type->kind != TYPE_UINT32 && decl_type->kind != TYPE_BYTE &&
            decl_type->kind != TYPE_CHAR)
        {
            type_error(&stmt->as.var_decl.name,
                       "sync modifier is only allowed on integer types (int, long, int32, uint, uint32, byte, char)");
        }
        else
        {
            /* Set sync_mod on the symbol */
            Symbol *symbol = symbol_table_lookup_symbol_current(table, stmt->as.var_decl.name);
            if (symbol != NULL)
            {
                symbol->sync_mod = SYNC_ATOMIC;
                DEBUG_VERBOSE("Set sync modifier on symbol: %.*s",
                              stmt->as.var_decl.name.length, stmt->as.var_decl.name.start);
            }
        }
    }

    // Check: nil can only be assigned to pointer types
    if (init_type && init_type->kind == TYPE_NIL && decl_type->kind != TYPE_POINTER)
    {
        type_error(&stmt->as.var_decl.name, "'nil' can only be assigned to pointer types");
        return;
    }

    // Allow assigning any concrete type to an 'any' variable (boxing)
    // Also allow assigning T[] to any[], T[][] to any[][], etc. (each element will be boxed)
    bool types_compatible = ast_type_equals(init_type, decl_type) ||
                           (decl_type->kind == TYPE_ANY && init_type != NULL);

    // Check for any[] assignment compatibility at any nesting level
    if (!types_compatible && decl_type->kind == TYPE_ARRAY && init_type != NULL && init_type->kind == TYPE_ARRAY)
    {
        // Walk down both types to find the innermost element types
        Type *decl_elem = decl_type->as.array.element_type;
        Type *init_elem = init_type->as.array.element_type;

        // Count nesting levels and check structure matches
        while (decl_elem != NULL && init_elem != NULL &&
               decl_elem->kind == TYPE_ARRAY && init_elem->kind == TYPE_ARRAY)
        {
            decl_elem = decl_elem->as.array.element_type;
            init_elem = init_elem->as.array.element_type;
        }

        // If decl's innermost element is any and init's innermost element is a concrete type, allow it
        if (decl_elem != NULL && decl_elem->kind == TYPE_ANY && init_elem != NULL)
        {
            types_compatible = true;
        }
        // Allow implicit narrowing for integer arrays (e.g., int[] to byte[])
        // This supports cases like: var arr: byte[] = {x, x + 1} where expressions promote to int
        // Note: Only integer-to-integer narrowing is allowed, not float/double to integer
        else if (decl_elem != NULL && init_elem != NULL &&
                 is_numeric_type(decl_elem) && is_numeric_type(init_elem) &&
                 decl_elem->kind != TYPE_DOUBLE && decl_elem->kind != TYPE_FLOAT &&
                 init_elem->kind != TYPE_DOUBLE && init_elem->kind != TYPE_FLOAT)
        {
            types_compatible = true;
        }
    }

    if (init_type && !types_compatible)
    {
        if (stmt->as.var_decl.initializer &&
            stmt->as.var_decl.initializer->type == EXPR_THREAD_SPAWN)
        {
            type_error(&stmt->as.var_decl.name,
                       "Thread spawn return type does not match variable type");
        }
        else if (stmt->as.var_decl.initializer &&
                 stmt->as.var_decl.initializer->type == EXPR_LAMBDA &&
                 stmt->as.var_decl.initializer->as.lambda.is_native &&
                 decl_type->kind == TYPE_FUNCTION &&
                 decl_type->as.function.is_native)
        {
            /* Native lambda assigned to native callback type with signature mismatch */
            if (init_type->kind == TYPE_FUNCTION &&
                init_type->as.function.param_count != decl_type->as.function.param_count)
            {
                type_error(&stmt->as.var_decl.name,
                           "Native lambda parameter count does not match callback type");
            }
            else if (init_type->kind == TYPE_FUNCTION &&
                     !ast_type_equals(init_type->as.function.return_type,
                                      decl_type->as.function.return_type))
            {
                type_error(&stmt->as.var_decl.name,
                           "Native lambda return type does not match callback type");
            }
            else
            {
                type_error(&stmt->as.var_decl.name,
                           "Native lambda signature does not match callback type");
            }
        }
        else
        {
            type_error(&stmt->as.var_decl.name, "Initializer type does not match variable type");
        }
    }

    // Mark variable as pending if initialized with a thread spawn (non-void)
    if (stmt->as.var_decl.initializer &&
        stmt->as.var_decl.initializer->type == EXPR_THREAD_SPAWN &&
        init_type && init_type->kind != TYPE_VOID)
    {
        Symbol *sym = symbol_table_lookup_symbol(table, stmt->as.var_decl.name);
        if (sym != NULL)
        {
            symbol_table_mark_pending(sym);

            /* Collect frozen arguments from the spawn call and store on pending symbol.
             * This allows unfreezing when the variable is synced. */
            Expr *spawn = stmt->as.var_decl.initializer;
            Expr *call = spawn->as.thread_spawn.call;

            /* Static calls (like Process.run) don't have frozen args to track */
            if (call != NULL && call->type == EXPR_STATIC_CALL)
            {
                /* No frozen args handling needed for static method spawns */
            }
            else if (call != NULL && call->type == EXPR_CALL)
            {
                int arg_count = call->as.call.arg_count;
                Expr **arguments = call->as.call.arguments;

                /* Get function type to access param_mem_quals for 'as ref' detection */
                Expr *callee = call->as.call.callee;
                Type *func_type = NULL;
                if (callee != NULL && callee->type == EXPR_VARIABLE)
                {
                    Symbol *func_sym = symbol_table_lookup_symbol(table, callee->as.variable.name);
                    if (func_sym != NULL && func_sym->type != NULL &&
                        func_sym->type->kind == TYPE_FUNCTION)
                    {
                        func_type = func_sym->type;
                    }
                }
                MemoryQualifier *param_quals = (func_type != NULL) ?
                    func_type->as.function.param_mem_quals : NULL;
                int param_count = (func_type != NULL) ?
                    func_type->as.function.param_count : 0;

                /* Count frozen args first (arrays, strings, and 'as ref' primitives) */
                int frozen_count = 0;
                for (int i = 0; i < arg_count; i++)
                {
                    Expr *arg = arguments[i];
                    if (arg != NULL && arg->type == EXPR_VARIABLE)
                    {
                        Symbol *arg_sym = symbol_table_lookup_symbol(table, arg->as.variable.name);
                        if (arg_sym != NULL && arg_sym->type != NULL)
                        {
                            /* Arrays and strings are always frozen */
                            if (arg_sym->type->kind == TYPE_ARRAY || arg_sym->type->kind == TYPE_STRING)
                            {
                                frozen_count++;
                            }
                            /* Primitives with 'as ref' are also frozen */
                            else if (param_quals != NULL && i < param_count &&
                                     param_quals[i] == MEM_AS_REF)
                            {
                                frozen_count++;
                            }
                        }
                    }
                }

                /* Allocate and fill frozen_args array */
                if (frozen_count > 0)
                {
                    Symbol **frozen_args = (Symbol **)arena_alloc(table->arena,
                                                                   sizeof(Symbol *) * frozen_count);
                    int idx = 0;
                    for (int i = 0; i < arg_count; i++)
                    {
                        Expr *arg = arguments[i];
                        if (arg != NULL && arg->type == EXPR_VARIABLE)
                        {
                            Symbol *arg_sym = symbol_table_lookup_symbol(table, arg->as.variable.name);
                            if (arg_sym != NULL && arg_sym->type != NULL)
                            {
                                /* Arrays and strings are always frozen */
                                if (arg_sym->type->kind == TYPE_ARRAY || arg_sym->type->kind == TYPE_STRING)
                                {
                                    frozen_args[idx++] = arg_sym;
                                }
                                /* Primitives with 'as ref' are also frozen */
                                else if (param_quals != NULL && i < param_count &&
                                         param_quals[i] == MEM_AS_REF)
                                {
                                    frozen_args[idx++] = arg_sym;
                                }
                            }
                        }
                    }
                    symbol_table_set_frozen_args(sym, frozen_args, frozen_count);
                }
            }
        }
    }
}

/* Add 'arena' built-in identifier to current scope.
 * This makes 'arena' available in non-native functions and methods,
 * allowing SDK code to pass the arena to native runtime functions. */
static void add_arena_builtin(SymbolTable *table, Token *ref_token)
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

/* Type-check only the function body, without adding to global scope.
 * Used for namespaced imports where the function is registered under a namespace. */
static void type_check_function_body_only(Stmt *stmt, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking function body only: %.*s", stmt->as.function.name.length, stmt->as.function.name.start);
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
}

static void type_check_function(Stmt *stmt, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking function with %d parameters", stmt->as.function.param_count);

    /* Enforce that pointer types in function signatures require native keyword.
     * Regular functions cannot have pointer parameters or return types. */
    if (!stmt->as.function.is_native)
    {
        /* Check return type for pointers */
        if (stmt->as.function.return_type &&
            stmt->as.function.return_type->kind == TYPE_POINTER)
        {
            type_error(&stmt->as.function.name,
                       "Pointer return type requires 'native' function");
            return;
        }

        /* Check parameter types for pointers */
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

    /* Special validation for main function parameters.
     * main() can optionally accept a single str[] parameter for command-line args. */
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
        /* Handle null parameter type - use NIL as placeholder */
        if (param_type == NULL) {
            param_type = ast_create_primitive_type(arena, TYPE_NIL);
        }
        param_types[i] = param_type;
    }
    Type *func_type = ast_create_function_type(arena, stmt->as.function.return_type, param_types, stmt->as.function.param_count);
    /* Carry over variadic flag from function statement */
    func_type->as.function.is_variadic = stmt->as.function.is_variadic;
    /* Carry over native flag so code gen emits direct C calls, not closure calls */
    func_type->as.function.is_native = stmt->as.function.is_native;
    /* Track whether function has a body (vs true extern declaration) */
    func_type->as.function.has_body = (stmt->as.function.body_count > 0);

    /* Store parameter memory qualifiers in the function type for thread safety analysis.
     * This allows detecting 'as ref' primitives when checking thread spawn arguments. */
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

    /* Functions returning heap-allocated types (closures, strings, arrays) must be
     * implicitly shared to avoid arena lifetime issues - the returned value must
     * live in caller's arena, not the function's arena which is destroyed on return.
     * This matches the implicit sharing logic in code_gen_stmt.c:301-307. */
    FunctionModifier effective_modifier = modifier;
    if (stmt->as.function.return_type &&
        (stmt->as.function.return_type->kind == TYPE_FUNCTION ||
         stmt->as.function.return_type->kind == TYPE_STRING ||
         stmt->as.function.return_type->kind == TYPE_ARRAY) &&
        modifier != FUNC_PRIVATE)
    {
        effective_modifier = FUNC_SHARED;
    }

    /* Check for duplicate function definition (collision from imports).
     * If a function with this name already exists, report a collision error. */
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

    /* Add function symbol to current scope (e.g., global) with its modifier.
     * We pass both the effective modifier (for code gen arena passing) and
     * the declared modifier (for thread spawn mode selection). */
    if (stmt->as.function.is_native)
    {
        symbol_table_add_native_function(table, stmt->as.function.name, func_type, effective_modifier, modifier);
    }
    else
    {
        symbol_table_add_function(table, stmt->as.function.name, func_type, effective_modifier, modifier);
    }

    /* Set c_alias on the symbol if the function has one (from #pragma alias) */
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

        /* Check for null parameter type - report error and use placeholder */
        if (param.type == NULL)
        {
            type_error(&param.name, "Parameter type is missing");
            param.type = ast_create_primitive_type(arena, TYPE_NIL);
        }

        /* Validate parameter memory qualifier */
        if (param.mem_qualifier == MEM_AS_VAL)
        {
            /* 'as val' on parameters is meaningful only for reference types */
            if (is_primitive_type(param.type))
            {
                DEBUG_VERBOSE("Warning: 'as val' on primitive parameter has no effect");
            }
        }
        else if (param.mem_qualifier == MEM_AS_REF)
        {
            /* 'as ref' on primitive parameters allows caller to pass a reference
             * that the function can modify. This enables shared mutable state.
             * 'as ref' on struct parameters passes a pointer to the struct,
             * matching C semantics for out-parameters (e.g., gettimeofday(tv, tz)). */
            if (!is_primitive_type(param.type) && param.type->kind != TYPE_STRUCT)
            {
                /* 'as ref' only makes sense for primitives and structs - arrays are already references */
                type_error(&param.name, "'as ref' only applies to primitive or struct parameters");
            }
        }

        /* Add symbol with the memory qualifier so code gen can handle dereferencing */
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

    /* Track native function context for pointer type restrictions.
     * Native functions can declare pointer variables, regular functions cannot. */
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
}

static void type_check_return(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking return statement");
    Type *value_type;
    if (stmt->as.return_stmt.value)
    {
        value_type = type_check_expr(stmt->as.return_stmt.value, table);
        if (value_type == NULL)
            return;

        /* Escape analysis: detect when returning a local variable.
         * Local variables (declared inside function, scope_depth >= 2) escape when returned.
         * Note: After symbol_table_init, global scope has depth 1. Function body scope is >= 2.
         * Parameters (SYMBOL_PARAM) and globals (scope_depth == 1) don't escape. */
        Expr *return_expr = stmt->as.return_stmt.value;
        if (return_expr->type == EXPR_VARIABLE)
        {
            Symbol *sym = symbol_table_lookup_symbol(table, return_expr->as.variable.name);
            if (sym != NULL && sym->kind != SYMBOL_PARAM && sym->declaration_scope_depth >= 2)
            {
                /* Local variable is escaping via return - mark it */
                ast_expr_mark_escapes(return_expr);
                DEBUG_VERBOSE("Escape detected: local variable '%.*s' (scope_depth %d) returned from function",
                              return_expr->as.variable.name.length,
                              return_expr->as.variable.name.start,
                              sym->declaration_scope_depth);
            }
        }
    }
    else
    {
        value_type = ast_create_primitive_type(table->arena, TYPE_VOID);
    }
    if (!ast_type_equals(value_type, return_type))
    {
        type_error(stmt->token, "Return type does not match function return type");
    }
}

static void type_check_block(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking block with %d statements", stmt->as.block.count);

    BlockModifier modifier = stmt->as.block.modifier;
    bool is_private = modifier == BLOCK_PRIVATE;

    if (is_private)
    {
        DEBUG_VERBOSE("Entering private block - escape analysis will be enforced");
        symbol_table_enter_arena(table);
    }
    else if (modifier == BLOCK_SHARED)
    {
        DEBUG_VERBOSE("Entering shared block - using parent's arena");
        /* Shared block: allocations use parent's arena, no special restrictions */
    }

    symbol_table_push_scope(table);
    for (int i = 0; i < stmt->as.block.count; i++)
    {
        type_check_stmt(stmt->as.block.statements[i], table, return_type);
    }
    symbol_table_pop_scope(table);

    if (is_private)
    {
        symbol_table_exit_arena(table);
    }
}

static void type_check_if(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking if statement");
    Type *cond_type = type_check_expr(stmt->as.if_stmt.condition, table);
    if (cond_type && cond_type->kind != TYPE_BOOL)
    {
        type_error(stmt->as.if_stmt.condition->token, "If condition must be boolean");
    }
    type_check_stmt(stmt->as.if_stmt.then_branch, table, return_type);
    if (stmt->as.if_stmt.else_branch)
    {
        DEBUG_VERBOSE("Type checking else branch");
        type_check_stmt(stmt->as.if_stmt.else_branch, table, return_type);
    }
}

static void type_check_while(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking while statement");
    Type *cond_type = type_check_expr(stmt->as.while_stmt.condition, table);
    if (cond_type && cond_type->kind != TYPE_BOOL)
    {
        type_error(stmt->as.while_stmt.condition->token, "While condition must be boolean");
    }

    // Non-shared loops have per-iteration arenas - enter arena context for escape analysis
    bool is_shared = stmt->as.while_stmt.is_shared;
    if (!is_shared)
    {
        symbol_table_enter_arena(table);
    }

    // Track loop context for break/continue validation
    symbol_table_enter_loop(table);
    type_check_stmt(stmt->as.while_stmt.body, table, return_type);
    symbol_table_exit_loop(table);

    if (!is_shared)
    {
        symbol_table_exit_arena(table);
    }
}

static void type_check_for(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking for statement");
    symbol_table_push_scope(table);
    if (stmt->as.for_stmt.initializer)
    {
        type_check_stmt(stmt->as.for_stmt.initializer, table, return_type);
    }
    if (stmt->as.for_stmt.condition)
    {
        Type *cond_type = type_check_expr(stmt->as.for_stmt.condition, table);
        if (cond_type && cond_type->kind != TYPE_BOOL)
        {
            type_error(stmt->as.for_stmt.condition->token, "For condition must be boolean");
        }
    }
    if (stmt->as.for_stmt.increment)
    {
        type_check_expr(stmt->as.for_stmt.increment, table);
    }

    // Non-shared loops have per-iteration arenas - enter arena context for escape analysis
    bool is_shared = stmt->as.for_stmt.is_shared;
    if (!is_shared)
    {
        symbol_table_enter_arena(table);
    }

    // Track loop context for break/continue validation
    symbol_table_enter_loop(table);
    type_check_stmt(stmt->as.for_stmt.body, table, return_type);
    symbol_table_exit_loop(table);

    if (!is_shared)
    {
        symbol_table_exit_arena(table);
    }

    symbol_table_pop_scope(table);
}

static void type_check_for_each(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking for-each statement");

    // Type check the iterable expression
    Type *iterable_type = type_check_expr(stmt->as.for_each_stmt.iterable, table);
    if (iterable_type == NULL)
    {
        return;
    }

    // Verify the iterable is an array type
    if (iterable_type->kind != TYPE_ARRAY)
    {
        type_error(stmt->as.for_each_stmt.iterable->token, "For-each iterable must be an array");
        return;
    }

    // Get the element type from the array
    Type *element_type = iterable_type->as.array.element_type;

    // Create a new scope and add the loop variable
    // Use SYMBOL_PARAM so it's not freed - loop var is a reference to array element, not owned
    symbol_table_push_scope(table);
    symbol_table_add_symbol_with_kind(table, stmt->as.for_each_stmt.var_name, element_type, SYMBOL_PARAM);

    // Non-shared loops have per-iteration arenas - enter arena context for escape analysis
    bool is_shared = stmt->as.for_each_stmt.is_shared;
    if (!is_shared)
    {
        symbol_table_enter_arena(table);
    }

    // Track loop context for break/continue validation
    symbol_table_enter_loop(table);
    // Type check the body
    type_check_stmt(stmt->as.for_each_stmt.body, table, return_type);
    symbol_table_exit_loop(table);

    if (!is_shared)
    {
        symbol_table_exit_arena(table);
    }

    symbol_table_pop_scope(table);
}

/* Type check an import statement.
 *
 * For non-namespaced imports (namespace == NULL):
 *   - Module symbols are added to global scope when their function definitions
 *     are type-checked (handled by type_check_function)
 *   - Collision detection happens in type_check_function
 *   - This function just logs for debugging purposes
 *
 * For namespaced imports (namespace != NULL):
 *   - Creates a namespace entry in the symbol table
 *   - Registers all function symbols from imported module under that namespace
 *   - Namespaced symbols are NOT added to global scope directly
 *   - They are only accessible via namespace.symbol syntax
 */
/* Type check a struct declaration.
 * Validates:
 * 1. All field types are valid (primitives, arrays, strings, or defined struct/opaque types)
 * 2. Pointer fields are only allowed in native structs
 * 3. Default value types match field types
 */
static void type_check_struct_decl(Stmt *stmt, SymbolTable *table)
{
    StructDeclStmt *struct_decl = &stmt->as.struct_decl;

    DEBUG_VERBOSE("Type checking struct declaration: %.*s with %d fields",
                  struct_decl->name.length, struct_decl->name.start,
                  struct_decl->field_count);

    /* Check each field */
    for (int i = 0; i < struct_decl->field_count; i++)
    {
        StructField *field = &struct_decl->fields[i];
        Type *field_type = field->type;

        if (field_type == NULL)
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "Field '%s' has no type", field->name);
            type_error(&struct_decl->name, msg);
            continue;
        }

        /* Validate the field type is resolvable */
        if (!is_valid_field_type(field_type, table))
        {
            char msg[512];
            /* Get struct name for error message */
            char struct_name_buf[128];
            int struct_name_len = struct_decl->name.length < 127 ? struct_decl->name.length : 127;
            memcpy(struct_name_buf, struct_decl->name.start, struct_name_len);
            struct_name_buf[struct_name_len] = '\0';

            /* Get type name for undefined struct types */
            const char *type_name = "unknown";
            if (field_type->kind == TYPE_STRUCT && field_type->as.struct_type.name != NULL)
            {
                type_name = field_type->as.struct_type.name;
            }

            snprintf(msg, sizeof(msg),
                     "In struct '%s': field '%s' has undefined type '%s'",
                     struct_name_buf, field->name, type_name);
            type_error(&struct_decl->name, msg);
            continue;
        }

        /* Pointer fields require native struct (already checked in parser, but double-check) */
        if (!struct_decl->is_native && field_type->kind == TYPE_POINTER)
        {
            char msg[512];
            /* Get struct name for error message */
            char sname[128];
            int sname_len = struct_decl->name.length < 127 ? struct_decl->name.length : 127;
            memcpy(sname, struct_decl->name.start, sname_len);
            sname[sname_len] = '\0';

            snprintf(msg, sizeof(msg),
                     "Pointer field '%s' not allowed in struct '%s'. "
                     "Use 'native struct' for structs with pointer fields:\n"
                     "    native struct %s =>\n"
                     "        %s: *...",
                     field->name, sname, sname, field->name);
            type_error(&struct_decl->name, msg);
        }

        /* Type check default value if present */
        if (field->default_value != NULL)
        {
            Type *default_type = type_check_expr(field->default_value, table);
            if (default_type != NULL && !ast_type_equals(default_type, field_type))
            {
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "Default value type does not match field '%s' type",
                         field->name);
                type_error(&struct_decl->name, msg);
            }
        }

        DEBUG_VERBOSE("  Field '%s' type validated", field->name);
    }

    /* Type check each method */
    for (int i = 0; i < struct_decl->method_count; i++)
    {
        StructMethod *method = &struct_decl->methods[i];

        DEBUG_VERBOSE("  Type checking method '%s' (static=%d, native=%d)",
                      method->name, method->is_static, method->is_native);

        /* Resolve forward references in method return type and parameter types */
        if (method->return_type != NULL)
        {
            method->return_type = resolve_struct_forward_reference(method->return_type, table);
        }
        for (int j = 0; j < method->param_count; j++)
        {
            if (method->params[j].type != NULL)
            {
                method->params[j].type = resolve_struct_forward_reference(method->params[j].type, table);
            }
        }

        /* For non-native methods, type check the body */
        if (!method->is_native && method->body != NULL)
        {
            /* Create a new scope for the method body */
            symbol_table_push_scope(table);

            /* Add 'arena' built-in identifier for non-native methods */
            add_arena_builtin(table, &struct_decl->name);

            /* For instance methods, add 'self' parameter to scope */
            if (!method->is_static)
            {
                /* Look up the struct type */
                Symbol *struct_sym = symbol_table_lookup_type(table, struct_decl->name);
                if (struct_sym != NULL && struct_sym->type != NULL)
                {
                    /* Create a 'self' token */
                    Token self_token;
                    self_token.start = "self";
                    self_token.length = 4;
                    self_token.line = struct_decl->name.line;
                    self_token.filename = struct_decl->name.filename;
                    self_token.type = TOKEN_IDENTIFIER;

                    /* For opaque handle types (native structs with c_alias),
                     * the struct type itself IS the pointer type, so 'self' should
                     * be the struct type directly. Otherwise, 'self' is a pointer
                     * to the struct type (allows modifications). */
                    Type *self_type;
                    if (struct_decl->is_native && struct_decl->c_alias != NULL)
                    {
                        /* Opaque handle type: self is the struct type itself */
                        self_type = struct_sym->type;
                    }
                    else
                    {
                        /* Regular struct: self is a pointer to the struct type */
                        self_type = ast_create_pointer_type(table->arena, struct_sym->type);
                    }
                    symbol_table_add_symbol(table, self_token, self_type);
                }
            }

            /* Add method parameters to scope */
            for (int j = 0; j < method->param_count; j++)
            {
                Parameter *param = &method->params[j];
                if (param->type != NULL)
                {
                    symbol_table_add_symbol_full(table, param->name, param->type, SYMBOL_PARAM, param->mem_qualifier);
                }
            }

            /* Enter method context to allow pointer-to-struct access for 'self' */
            method_context_enter();

            /* Type check the method body */
            for (int j = 0; j < method->body_count; j++)
            {
                if (method->body[j] != NULL)
                {
                    type_check_stmt(method->body[j], table, method->return_type);
                }
            }

            /* Exit method context */
            method_context_exit();

            /* TODO: Check return type matches declared return type */

            symbol_table_pop_scope(table);
        }
    }

    /* Check for circular dependencies in struct types.
     * Build a temporary Type for the struct declaration to check. */
    Type temp_struct_type;
    temp_struct_type.kind = TYPE_STRUCT;
    temp_struct_type.as.struct_type.name = NULL;

    /* Create null-terminated struct name */
    char struct_name[128];
    int name_len = struct_decl->name.length < 127 ? struct_decl->name.length : 127;
    memcpy(struct_name, struct_decl->name.start, name_len);
    struct_name[name_len] = '\0';
    temp_struct_type.as.struct_type.name = struct_name;

    temp_struct_type.as.struct_type.fields = struct_decl->fields;
    temp_struct_type.as.struct_type.field_count = struct_decl->field_count;
    temp_struct_type.as.struct_type.methods = struct_decl->methods;
    temp_struct_type.as.struct_type.method_count = struct_decl->method_count;
    temp_struct_type.as.struct_type.is_native = struct_decl->is_native;
    temp_struct_type.as.struct_type.size = 0;
    temp_struct_type.as.struct_type.alignment = 0;

    char cycle_chain[512];
    if (detect_struct_circular_dependency(&temp_struct_type, table, cycle_chain, sizeof(cycle_chain)))
    {
        char msg[768];
        snprintf(msg, sizeof(msg),
                 "Circular dependency detected in struct '%s': %s",
                 struct_name, cycle_chain);
        type_error(&struct_decl->name, msg);
        return; /* Cannot calculate layout for circular struct */
    }

    /* Look up the struct type from the symbol table and calculate its layout */
    Symbol *struct_sym = symbol_table_lookup_type(table, struct_decl->name);
    if (struct_sym != NULL && struct_sym->type != NULL && struct_sym->type->kind == TYPE_STRUCT)
    {
        calculate_struct_layout(struct_sym->type);
        DEBUG_VERBOSE("Struct '%s' layout: size=%zu, alignment=%zu",
                      struct_name,
                      struct_sym->type->as.struct_type.size,
                      struct_sym->type->as.struct_type.alignment);
    }
}

static void type_check_import_stmt(Stmt *stmt, SymbolTable *table)
{
    ImportStmt *import = &stmt->as.import;

    char mod_str[128];
    int mod_len = import->module_name.length < 127 ? import->module_name.length : 127;
    memcpy(mod_str, import->module_name.start, mod_len);
    mod_str[mod_len] = '\0';

    if (import->namespace == NULL)
    {
        /* Non-namespaced import: symbols are added to global scope when
         * the imported function definitions are type-checked. The parser
         * merges imported statements into the main module, and collision
         * detection is handled by type_check_function when those merged
         * function statements are processed. */
        DEBUG_VERBOSE("Type checking non-namespaced import of '%s'", mod_str);
    }
    else
    {
        /* Namespaced import: create namespace and register symbols */
        Token ns_token = *import->namespace;
        char ns_str[128];
        int ns_len = ns_token.length < 127 ? ns_token.length : 127;
        memcpy(ns_str, ns_token.start, ns_len);
        ns_str[ns_len] = '\0';
        DEBUG_VERBOSE("Type checking namespaced import of '%s' as '%s'", mod_str, ns_str);

        /* Check if namespace identifier is a reserved keyword */
        const char *reserved = is_reserved_keyword(ns_token);
        if (reserved != NULL)
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "Cannot use reserved keyword '%s' as namespace name", reserved);
            type_error(&ns_token, msg);
            return;
        }

        /* Check if namespace already exists */
        if (symbol_table_is_namespace(table, ns_token))
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "Namespace '%s' is already defined", ns_str);
            type_error(&ns_token, msg);
            return;
        }

        /* Also check if a non-namespace symbol with this name exists */
        Symbol *existing = symbol_table_lookup_symbol(table, ns_token);
        if (existing != NULL)
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "Cannot use '%s' as namespace: name already in use", ns_str);
            type_error(&ns_token, msg);
            return;
        }

        /* Create the namespace entry in the symbol table */
        symbol_table_add_namespace(table, ns_token);

        /* Use get_module_symbols to extract symbols and types from imported module.
         * Create a temporary Module structure to use with the helper function. */
        Module temp_module;
        temp_module.statements = import->imported_stmts;
        temp_module.count = import->imported_count;
        temp_module.capacity = import->imported_count;
        temp_module.filename = NULL;

        Token **symbols = NULL;
        Type **types = NULL;
        int symbol_count = 0;

        get_module_symbols(&temp_module, table, &symbols, &types, &symbol_count);

        /* Handle empty modules gracefully */
        if (symbol_count == 0)
        {
            DEBUG_VERBOSE("No symbols to import from module '%s'", mod_str);
            return;
        }

        /* PASS 1: Register all symbols in namespace AND temporarily in global scope.
         * We need them in global scope temporarily so that functions within the
         * imported module can reference each other (e.g., a helper calling a native fn).
         * We need to iterate through original statements to get function modifiers
         * since get_module_symbols only extracts names and types.
         *
         * Track which symbols we add so we only remove those (not symbols from
         * a direct import of the same module). */
        int sym_idx = 0;
        bool *added_to_global = arena_alloc(table->arena, sizeof(bool) * symbol_count);
        for (int j = 0; j < symbol_count; j++)
            added_to_global[j] = false;

        int added_idx = 0;
        for (int i = 0; i < import->imported_count && sym_idx < symbol_count; i++)
        {
            Stmt *imported_stmt = import->imported_stmts[i];
            if (imported_stmt == NULL)
                continue;

            if (imported_stmt->type == STMT_FUNCTION)
            {
                FunctionStmt *func = &imported_stmt->as.function;

                /* Use the type extracted by get_module_symbols */
                Type *func_type = types[sym_idx];
                Token *func_name = symbols[sym_idx];
                sym_idx++;

                /* Determine effective modifier - same logic as type_check_function.
                 * Functions returning heap-allocated types are implicitly shared. */
                FunctionModifier modifier = func->modifier;
                FunctionModifier effective_modifier = modifier;
                if (func->return_type &&
                    (func->return_type->kind == TYPE_FUNCTION ||
                     func->return_type->kind == TYPE_STRING ||
                     func->return_type->kind == TYPE_ARRAY) &&
                    modifier != FUNC_PRIVATE)
                {
                    effective_modifier = FUNC_SHARED;
                }

                /* Add function symbol to namespace with proper function modifier */
                symbol_table_add_function_to_namespace(table, ns_token, *func_name, func_type, effective_modifier, modifier);

                /* Set c_alias on the namespace symbol if the function has one */
                if (func->c_alias != NULL)
                {
                    Symbol *ns_func_sym = symbol_table_lookup_in_namespace(table, ns_token, *func_name);
                    if (ns_func_sym != NULL)
                    {
                        ns_func_sym->c_alias = func->c_alias;
                    }
                }

                /* Only add to global scope if not already there (e.g., from direct import).
                 * Track whether we added it so we can remove it later.
                 * Use the appropriate function for native vs non-native functions. */
                Symbol *existing = symbol_table_lookup_symbol(table, *func_name);
                if (existing == NULL)
                {
                    if (func->is_native)
                    {
                        symbol_table_add_native_function(table, *func_name, func_type, effective_modifier, modifier);
                    }
                    else
                    {
                        symbol_table_add_function(table, *func_name, func_type, effective_modifier, modifier);
                    }
                    /* Set c_alias on the global symbol if the function has one */
                    if (func->c_alias != NULL)
                    {
                        Symbol *global_func_sym = symbol_table_lookup_symbol_current(table, *func_name);
                        if (global_func_sym != NULL)
                        {
                            global_func_sym->c_alias = func->c_alias;
                        }
                    }
                    added_to_global[added_idx] = true;
                }
                else
                {
                    /* Symbol already exists (added by parser). Update its function flags
                     * so code gen knows it's a named function, not a closure.
                     * Also update the type to ensure has_body is set correctly. */
                    existing->type = ast_clone_type(table->arena, func_type);
                    existing->is_function = true;
                    existing->is_native = func->is_native;
                    existing->func_mod = effective_modifier;
                    existing->declared_func_mod = modifier;
                    existing->c_alias = func->c_alias;
                }
                added_idx++;

                char func_str[128];
                int func_len = func_name->length < 127 ? func_name->length : 127;
                memcpy(func_str, func_name->start, func_len);
                func_str[func_len] = '\0';
                DEBUG_VERBOSE("Added function '%s' to namespace '%s' (mod=%d)", func_str, ns_str, effective_modifier);
            }
        }

        /* PASS 2: Type-check all function bodies.
         * Now all symbols are visible in global scope, so intra-module calls work. */
        for (int i = 0; i < import->imported_count; i++)
        {
            Stmt *imported_stmt = import->imported_stmts[i];
            if (imported_stmt == NULL)
                continue;

            if (imported_stmt->type == STMT_FUNCTION)
            {
                /* Type-check the function body so expr_type is set for code generation.
                 * Use the body-only version to avoid re-adding to global scope. */
                type_check_function_body_only(imported_stmt, table);
            }
        }

        /* PASS 3: Remove only the symbols WE added to global scope.
         * Don't remove symbols that were already there (e.g., from direct import). */
        sym_idx = 0;
        added_idx = 0;
        for (int i = 0; i < import->imported_count && sym_idx < symbol_count; i++)
        {
            Stmt *imported_stmt = import->imported_stmts[i];
            if (imported_stmt == NULL)
                continue;

            if (imported_stmt->type == STMT_FUNCTION)
            {
                Token *func_name = symbols[sym_idx];
                sym_idx++;
                if (added_to_global[added_idx])
                {
                    symbol_table_remove_symbol_from_global(table, *func_name);
                }
                added_idx++;
            }
        }
    }
}

void type_check_stmt(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    if (stmt == NULL)
    {
        DEBUG_VERBOSE("Statement is NULL");
        return;
    }
    DEBUG_VERBOSE("Type checking statement type: %d", stmt->type);
    switch (stmt->type)
    {
    case STMT_EXPR:
        type_check_expr(stmt->as.expression.expression, table);
        break;
    case STMT_VAR_DECL:
        type_check_var_decl(stmt, table, return_type);
        break;
    case STMT_FUNCTION:
        type_check_function(stmt, table);
        break;
    case STMT_RETURN:
        type_check_return(stmt, table, return_type);
        break;
    case STMT_BLOCK:
        type_check_block(stmt, table, return_type);
        break;
    case STMT_IF:
        type_check_if(stmt, table, return_type);
        break;
    case STMT_WHILE:
        type_check_while(stmt, table, return_type);
        break;
    case STMT_FOR:
        type_check_for(stmt, table, return_type);
        break;
    case STMT_FOR_EACH:
        type_check_for_each(stmt, table, return_type);
        break;
    case STMT_BREAK:
        DEBUG_VERBOSE("Type checking break statement");
        if (!symbol_table_in_loop(table))
        {
            type_error(stmt->token, "'break' statement must be inside a loop");
        }
        break;
    case STMT_CONTINUE:
        DEBUG_VERBOSE("Type checking continue statement");
        if (!symbol_table_in_loop(table))
        {
            type_error(stmt->token, "'continue' statement must be inside a loop");
        }
        break;
    case STMT_IMPORT:
        type_check_import_stmt(stmt, table);
        break;
    case STMT_PRAGMA:
        /* Pragma statements don't require type checking */
        DEBUG_VERBOSE("Type checking pragma statement (no-op)");
        break;
    case STMT_TYPE_DECL:
        /* Type declarations are already registered in the symbol table during parsing.
         * We just validate the type is an opaque type or native function type here. */
        DEBUG_VERBOSE("Type checking type declaration: %.*s",
                      stmt->as.type_decl.name.length, stmt->as.type_decl.name.start);
        /* Validate the type is an opaque type or native function type */
        if (stmt->as.type_decl.type == NULL)
        {
            type_error(&stmt->as.type_decl.name, "Type declaration must have a type");
        }
        else if (stmt->as.type_decl.type->kind == TYPE_OPAQUE)
        {
            /* Valid opaque type declaration */
        }
        else if (stmt->as.type_decl.type->kind == TYPE_FUNCTION &&
                 stmt->as.type_decl.type->as.function.is_native)
        {
            /* Valid native callback type declaration.
             * Validate that all parameter types and return type are C-compatible. */
            Type *func_type = stmt->as.type_decl.type;
            for (int i = 0; i < func_type->as.function.param_count; i++)
            {
                Type *param_type = func_type->as.function.param_types[i];
                if (!is_c_compatible_type(param_type))
                {
                    type_error(&stmt->as.type_decl.name,
                               "Native callback parameter type must be C-compatible (primitives, pointers, or opaque types)");
                    break;
                }
            }
            if (!is_c_compatible_type(func_type->as.function.return_type))
            {
                type_error(&stmt->as.type_decl.name,
                           "Native callback return type must be C-compatible (primitives, pointers, or opaque types)");
            }
        }
        else
        {
            type_error(&stmt->as.type_decl.name,
                       "Type declaration must be 'opaque' or 'native fn(...)'");
        }
        break;

    case STMT_STRUCT_DECL:
        type_check_struct_decl(stmt, table);
        break;

    case STMT_LOCK:
        DEBUG_VERBOSE("Type checking lock statement");
        /* Type check the lock expression */
        type_check_expr(stmt->as.lock_stmt.lock_expr, table);

        /* Validate that the lock expression is a sync variable */
        if (stmt->as.lock_stmt.lock_expr->type == EXPR_VARIABLE)
        {
            Symbol *lock_sym = symbol_table_lookup_symbol(table, stmt->as.lock_stmt.lock_expr->as.variable.name);
            if (lock_sym == NULL)
            {
                type_error(stmt->as.lock_stmt.lock_expr->token, "Undefined variable in lock expression");
            }
            else if (lock_sym->sync_mod != SYNC_ATOMIC)
            {
                type_error(stmt->as.lock_stmt.lock_expr->token,
                           "Lock expression must be a sync variable");
            }
        }
        else
        {
            type_error(stmt->as.lock_stmt.lock_expr->token,
                       "Lock expression must be a sync variable");
        }

        /* Type check the body */
        type_check_stmt(stmt->as.lock_stmt.body, table, return_type);
        break;
    }
}
