#include "parser/parser_stmt_decl.h"
#include "parser/parser_util.h"
#include "parser/parser_expr.h"
#include "parser/parser_stmt.h"
#include "ast/ast_expr.h"
#include "ast/ast_stmt.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

/* Helper function to check if a token type can start an expression */
static int parser_can_start_expression(SnTokenType type)
{
    switch (type)
    {
        /* Literals */
        case TOKEN_INT_LITERAL:
        case TOKEN_LONG_LITERAL:
        case TOKEN_BYTE_LITERAL:
        case TOKEN_DOUBLE_LITERAL:
        case TOKEN_FLOAT_LITERAL:
        case TOKEN_UINT_LITERAL:
        case TOKEN_UINT32_LITERAL:
        case TOKEN_INT32_LITERAL:
        case TOKEN_CHAR_LITERAL:
        case TOKEN_STRING_LITERAL:
        case TOKEN_INTERPOL_STRING:
        case TOKEN_BOOL_LITERAL:
        case TOKEN_NIL:
        /* Identifiers and lambdas */
        case TOKEN_IDENTIFIER:
        case TOKEN_FN:
        /* Grouping and array literals */
        case TOKEN_LEFT_PAREN:
        case TOKEN_LEFT_BRACE:
        /* Unary operators */
        case TOKEN_BANG:
        case TOKEN_MINUS:
        /* Type operators */
        case TOKEN_TYPEOF:
        case TOKEN_SIZEOF:
        /* Function reference */
        case TOKEN_AMPERSAND:
            return 1;
        default:
            return 0;
    }
}

Stmt *parser_var_declaration(Parser *parser)
{
    Token var_token = parser->previous;
    Token name;
    if (parser_check(parser, TOKEN_IDENTIFIER))
    {
        name = parser->current;
        parser_advance(parser);
        name.start = arena_strndup(parser->arena, name.start, name.length);
        if (name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
    }
    else
    {
        parser_error_at_current(parser, "Expected variable name");
        name = parser->current;
        parser_advance(parser);
        name.start = arena_strndup(parser->arena, name.start, name.length);
        if (name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
    }

    // Type annotation is optional if there's an initializer (type inference)
    Type *type = NULL;
    MemoryQualifier mem_qualifier = MEM_DEFAULT;
    SyncModifier sync_modifier = SYNC_NONE;
    Expr *sized_array_size_expr = NULL;
    if (parser_match(parser, TOKEN_COLON))
    {
        // Check for sync modifier before type
        if (parser_match(parser, TOKEN_SYNC))
        {
            sync_modifier = SYNC_ATOMIC;
        }

        ParsedType parsed = parser_type_with_size(parser);
        type = parsed.type;

        /* Check if this is a sized array type (e.g., int[10]) */
        if (parsed.is_sized_array)
        {
            sized_array_size_expr = parsed.size_expr;
        }

        // Parse optional "as val" or "as ref" after type
        mem_qualifier = parser_memory_qualifier(parser);
    }

    Expr *initializer = NULL;
    if (parser_match(parser, TOKEN_EQUAL))
    {
        initializer = parser_expression(parser);
    }

    /* If this is a sized array declaration, create the sized array alloc expression */
    if (sized_array_size_expr != NULL)
    {
        Expr *default_value = initializer; /* The initializer becomes the default value */
        initializer = ast_create_sized_array_alloc_expr(
            parser->arena, type, sized_array_size_expr, default_value, &var_token);
        /* The variable type becomes an array of the element type */
        type = ast_create_array_type(parser->arena, type);
    }

    // Must have either type annotation or initializer (or both)
    if (type == NULL && initializer == NULL)
    {
        parser_error_at_current(parser, "Variable declaration requires type annotation or initializer");
    }

    /* After multi-line lambda with statement body, we may be at the next statement already
     * (no NEWLINE token between DEDENT and the next statement). Also handle DEDENT case
     * when the var declaration is the last statement in a block. */
    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_match(parser, TOKEN_NEWLINE))
    {
        /* Allow if we're at the next statement (IDENTIFIER covers print, etc.),
         * a keyword that starts a statement, end of block (DEDENT), or EOF */
        if (!parser_check(parser, TOKEN_IDENTIFIER) && !parser_check(parser, TOKEN_VAR) &&
            !parser_check(parser, TOKEN_FN) && !parser_check(parser, TOKEN_IF) &&
            !parser_check(parser, TOKEN_WHILE) && !parser_check(parser, TOKEN_FOR) &&
            !parser_check(parser, TOKEN_RETURN) && !parser_check(parser, TOKEN_BREAK) &&
            !parser_check(parser, TOKEN_CONTINUE) &&
            !parser_check(parser, TOKEN_DEDENT) && !parser_check(parser, TOKEN_EOF))
        {
            parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after variable declaration");
        }
    }

    Stmt *stmt = ast_create_var_decl_stmt(parser->arena, name, type, initializer, &var_token);
    if (stmt != NULL)
    {
        stmt->as.var_decl.mem_qualifier = mem_qualifier;
        stmt->as.var_decl.sync_modifier = sync_modifier;
    }
    return stmt;
}

Stmt *parser_function_declaration(Parser *parser)
{
    Token fn_token = parser->previous;
    Token name;
    if (parser_check(parser, TOKEN_IDENTIFIER))
    {
        name = parser->current;
        parser_advance(parser);
        name.start = arena_strndup(parser->arena, name.start, name.length);
        if (name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
    }
    else
    {
        parser_error_at_current(parser, "Expected function name");
        name = parser->current;
        parser_advance(parser);
        name.start = arena_strndup(parser->arena, name.start, name.length);
        if (name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
    }

    Parameter *params = NULL;
    int param_count = 0;
    int param_capacity = 0;

    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        if (!parser_check(parser, TOKEN_RIGHT_PAREN))
        {
            do
            {
                if (param_count >= 255)
                {
                    parser_error_at_current(parser, "Cannot have more than 255 parameters");
                }
                Token param_name;
                if (parser_check(parser, TOKEN_IDENTIFIER))
                {
                    param_name = parser->current;
                    parser_advance(parser);
                    param_name.start = arena_strndup(parser->arena, param_name.start, param_name.length);
                    if (param_name.start == NULL)
                    {
                        parser_error_at_current(parser, "Out of memory");
                        return NULL;
                    }
                }
                else
                {
                    parser_error_at_current(parser, "Expected parameter name");
                    param_name = parser->current;
                    parser_advance(parser);
                    param_name.start = arena_strndup(parser->arena, param_name.start, param_name.length);
                    if (param_name.start == NULL)
                    {
                        parser_error_at_current(parser, "Out of memory");
                        return NULL;
                    }
                }
                parser_consume(parser, TOKEN_COLON, "Expected ':' after parameter name");
                // Check for sync modifier before type
                SyncModifier param_sync = SYNC_NONE;
                if (parser_match(parser, TOKEN_SYNC))
                {
                    param_sync = SYNC_ATOMIC;
                }
                Type *param_type = parser_type(parser);
                // Parse optional "as val" for parameter
                MemoryQualifier param_qualifier = parser_memory_qualifier(parser);

                if (param_count >= param_capacity)
                {
                    param_capacity = param_capacity == 0 ? 8 : param_capacity * 2;
                    Parameter *new_params = arena_alloc(parser->arena, sizeof(Parameter) * param_capacity);
                    if (new_params == NULL)
                    {
                        exit(1);
                    }
                    if (params != NULL && param_count > 0)
                    {
                        memcpy(new_params, params, sizeof(Parameter) * param_count);
                    }
                    params = new_params;
                }
                params[param_count].name = param_name;
                params[param_count].type = param_type;
                params[param_count].mem_qualifier = param_qualifier;
                params[param_count].sync_modifier = param_sync;
                param_count++;
            } while (parser_match(parser, TOKEN_COMMA));
        }
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");
    }

    // Parse optional function modifier (shared/private) before return type
    FunctionModifier func_modifier = parser_function_modifier(parser);

    Type *return_type = ast_create_primitive_type(parser->arena, TYPE_VOID);
    if (parser_match(parser, TOKEN_COLON))
    {
        return_type = parser_type(parser);
    }

    Type **param_types = arena_alloc(parser->arena, sizeof(Type *) * param_count);
    if (param_types == NULL && param_count > 0)
    {
        exit(1);
    }
    for (int i = 0; i < param_count; i++)
    {
        param_types[i] = params[i].type;
    }
    Type *function_type = ast_create_function_type(parser->arena, return_type, param_types, param_count);

    symbol_table_add_symbol(parser->symbol_table, name, function_type);

    parser_consume(parser, TOKEN_ARROW, "Expected '=>' before function body");

    /* Check for expression-bodied function (expression on same line as arrow) */
    Token arrow_token = parser->previous;
    Stmt **stmts = NULL;
    int stmt_count = 0;

    if (parser->current.line == arrow_token.line &&
        parser_can_start_expression(parser->current.type))
    {
        /* Expression-bodied function: fn foo(): int => 42 */
        Expr *body_expr = parser_expression(parser);

        /* Create an implicit return statement wrapping the expression */
        Stmt *return_stmt = ast_create_return_stmt(parser->arena, arrow_token, body_expr, &arrow_token);

        stmts = arena_alloc(parser->arena, sizeof(Stmt *));
        if (stmts == NULL)
        {
            parser_error(parser, "Out of memory");
            return NULL;
        }
        stmts[0] = return_stmt;
        stmt_count = 1;
    }
    else
    {
        /* Block-bodied function */
        skip_newlines(parser);

        Stmt *body = parser_indented_block(parser);
        if (body == NULL)
        {
            body = ast_create_block_stmt(parser->arena, NULL, 0, NULL);
        }

        stmts = body->as.block.statements;
        stmt_count = body->as.block.count;
        body->as.block.statements = NULL;
    }

    Stmt *func_stmt = ast_create_function_stmt(parser->arena, name, params, param_count, return_type, stmts, stmt_count, &fn_token);
    if (func_stmt != NULL)
    {
        func_stmt->as.function.modifier = func_modifier;
    }
    return func_stmt;
}

Stmt *parser_native_function_declaration(Parser *parser)
{
    Token native_token = parser->previous;  /* Points to 'fn' after 'native' */
    Token name;
    if (parser_check(parser, TOKEN_IDENTIFIER))
    {
        name = parser->current;
        parser_advance(parser);
        name.start = arena_strndup(parser->arena, name.start, name.length);
        if (name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
    }
    else
    {
        parser_error_at_current(parser, "Expected function name");
        name = parser->current;
        parser_advance(parser);
        name.start = arena_strndup(parser->arena, name.start, name.length);
        if (name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
    }

    Parameter *params = NULL;
    int param_count = 0;
    int param_capacity = 0;
    bool is_variadic = false;

    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        if (!parser_check(parser, TOKEN_RIGHT_PAREN))
        {
            do
            {
                /* Check for variadic '...' - must be last in parameter list */
                if (parser_match(parser, TOKEN_SPREAD))
                {
                    is_variadic = true;
                    /* '...' must be followed by ')' - no more parameters allowed */
                    break;
                }

                if (param_count >= 255)
                {
                    parser_error_at_current(parser, "Cannot have more than 255 parameters");
                }
                Token param_name;
                if (parser_check(parser, TOKEN_IDENTIFIER))
                {
                    param_name = parser->current;
                    parser_advance(parser);
                    param_name.start = arena_strndup(parser->arena, param_name.start, param_name.length);
                    if (param_name.start == NULL)
                    {
                        parser_error_at_current(parser, "Out of memory");
                        return NULL;
                    }
                }
                else
                {
                    parser_error_at_current(parser, "Expected parameter name");
                    param_name = parser->current;
                    parser_advance(parser);
                    param_name.start = arena_strndup(parser->arena, param_name.start, param_name.length);
                    if (param_name.start == NULL)
                    {
                        parser_error_at_current(parser, "Out of memory");
                        return NULL;
                    }
                }
                parser_consume(parser, TOKEN_COLON, "Expected ':' after parameter name");
                /* Check for sync modifier before type */
                SyncModifier param_sync = SYNC_NONE;
                if (parser_match(parser, TOKEN_SYNC))
                {
                    param_sync = SYNC_ATOMIC;
                }
                Type *param_type = parser_type(parser);
                /* Parse optional "as val" for parameter */
                MemoryQualifier param_qualifier = parser_memory_qualifier(parser);

                if (param_count >= param_capacity)
                {
                    param_capacity = param_capacity == 0 ? 8 : param_capacity * 2;
                    Parameter *new_params = arena_alloc(parser->arena, sizeof(Parameter) * param_capacity);
                    if (new_params == NULL)
                    {
                        exit(1);
                    }
                    if (params != NULL && param_count > 0)
                    {
                        memcpy(new_params, params, sizeof(Parameter) * param_count);
                    }
                    params = new_params;
                }
                params[param_count].name = param_name;
                params[param_count].type = param_type;
                params[param_count].mem_qualifier = param_qualifier;
                params[param_count].sync_modifier = param_sync;
                param_count++;
            } while (parser_match(parser, TOKEN_COMMA));
        }
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");
    }

    /* Parse optional function modifier (shared/private) before return type */
    FunctionModifier func_modifier = parser_function_modifier(parser);

    Type *return_type = ast_create_primitive_type(parser->arena, TYPE_VOID);
    if (parser_match(parser, TOKEN_COLON))
    {
        return_type = parser_type(parser);
    }

    Type **param_types = arena_alloc(parser->arena, sizeof(Type *) * param_count);
    if (param_types == NULL && param_count > 0)
    {
        exit(1);
    }
    for (int i = 0; i < param_count; i++)
    {
        param_types[i] = params[i].type;
    }
    Type *function_type = ast_create_function_type(parser->arena, return_type, param_types, param_count);
    /* Mark function type as variadic and native */
    function_type->as.function.is_variadic = is_variadic;
    function_type->as.function.is_native = true;  /* Native functions need is_native on the type too */

    symbol_table_add_symbol(parser->symbol_table, name, function_type);

    Stmt **stmts = NULL;
    int stmt_count = 0;

    /* Native functions can have either:
     * 1. No body (just declaration) - ends with newline or semicolon
     * 2. A Sindarin body using '=>' (native fn with Sindarin implementation)
     *    This can be expression-bodied (same line) or block-bodied (indented)
     */
    if (parser_match(parser, TOKEN_ARROW))
    {
        Token arrow_token = parser->previous;

        /* Set native context so lambdas parsed in body are marked as native */
        int saved_in_native = parser->in_native_function;
        parser->in_native_function = 1;

        /* Check for expression-bodied native function (expression on same line as arrow) */
        if (parser->current.line == arrow_token.line &&
            parser_can_start_expression(parser->current.type))
        {
            /* Expression-bodied native function: native fn foo(): int => 42 */
            Expr *body_expr = parser_expression(parser);

            /* Create an implicit return statement wrapping the expression */
            Stmt *return_stmt = ast_create_return_stmt(parser->arena, arrow_token, body_expr, &arrow_token);

            stmts = arena_alloc(parser->arena, sizeof(Stmt *));
            if (stmts == NULL)
            {
                parser_error(parser, "Out of memory");
                parser->in_native_function = saved_in_native;
                return NULL;
            }
            stmts[0] = return_stmt;
            stmt_count = 1;
        }
        else
        {
            /* Block-bodied native function */
            skip_newlines(parser);

            Stmt *body = parser_indented_block(parser);
            if (body == NULL)
            {
                body = ast_create_block_stmt(parser->arena, NULL, 0, NULL);
            }

            stmts = body->as.block.statements;
            stmt_count = body->as.block.count;
            body->as.block.statements = NULL;
        }

        /* Restore native context */
        parser->in_native_function = saved_in_native;
    }
    else
    {
        /* Native function without body (external C function declaration) */
        if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_match(parser, TOKEN_NEWLINE))
        {
            if (!parser_check(parser, TOKEN_NEWLINE) && !parser_check(parser, TOKEN_EOF) &&
                !parser_check(parser, TOKEN_FN) && !parser_check(parser, TOKEN_NATIVE) &&
                !parser_check(parser, TOKEN_VAR) && !parser_check(parser, TOKEN_DEDENT))
            {
                parser_consume(parser, TOKEN_NEWLINE, "Expected newline or '=>' after native function signature");
            }
        }
    }

    Stmt *func_stmt = ast_create_function_stmt(parser->arena, name, params, param_count, return_type, stmts, stmt_count, &native_token);
    if (func_stmt != NULL)
    {
        func_stmt->as.function.modifier = func_modifier;
        func_stmt->as.function.is_native = true;
        func_stmt->as.function.is_variadic = is_variadic;

        /* Handle #pragma alias for native functions */
        if (parser->pending_alias != NULL)
        {
            func_stmt->as.function.c_alias = parser->pending_alias;
            parser->pending_alias = NULL;
        }
    }
    return func_stmt;
}

/* Parse a native function type: native fn(params): return_type */
Type *parser_native_function_type(Parser *parser)
{
    /* Consume 'fn' after 'native' */
    parser_consume(parser, TOKEN_FN, "Expected 'fn' after 'native' in type declaration");
    parser_consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'fn' in native function type");

    Type **param_types = NULL;
    int param_count = 0;
    int param_capacity = 0;

    if (!parser_check(parser, TOKEN_RIGHT_PAREN))
    {
        do
        {
            /* For native function types, we parse: param_name: type */
            /* Skip the parameter name if present */
            if (parser_check(parser, TOKEN_IDENTIFIER))
            {
                Token param_name = parser->current;
                parser_advance(parser);

                /* Check if followed by colon (named parameter) or it's a type name */
                if (parser_check(parser, TOKEN_COLON))
                {
                    parser_advance(parser); /* consume ':' */
                    /* Now parse the actual type */
                }
                else
                {
                    /* It was a type name, need to back up - but we can't.
                     * Instead, create the type from the identifier.
                     * Check if it's a known type */
                    Symbol *type_symbol = symbol_table_lookup_type(parser->symbol_table, param_name);
                    if (type_symbol != NULL && type_symbol->type != NULL)
                    {
                        Type *param_type = ast_clone_type(parser->arena, type_symbol->type);
                        if (param_count >= param_capacity)
                        {
                            param_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
                            Type **new_params = arena_alloc(parser->arena, sizeof(Type *) * param_capacity);
                            if (new_params == NULL)
                            {
                                parser_error(parser, "Out of memory");
                                return NULL;
                            }
                            if (param_types != NULL && param_count > 0)
                            {
                                memcpy(new_params, param_types, sizeof(Type *) * param_count);
                            }
                            param_types = new_params;
                        }
                        param_types[param_count++] = param_type;
                        continue;
                    }
                    else
                    {
                        parser_error_at_current(parser, "Expected ':' after parameter name in native function type");
                        return NULL;
                    }
                }
            }

            Type *param_type = parser_type(parser);
            if (param_count >= param_capacity)
            {
                param_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
                Type **new_params = arena_alloc(parser->arena, sizeof(Type *) * param_capacity);
                if (new_params == NULL)
                {
                    parser_error(parser, "Out of memory");
                    return NULL;
                }
                if (param_types != NULL && param_count > 0)
                {
                    memcpy(new_params, param_types, sizeof(Type *) * param_count);
                }
                param_types = new_params;
            }
            param_types[param_count++] = param_type;
        } while (parser_match(parser, TOKEN_COMMA));
    }

    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameter types");
    parser_consume(parser, TOKEN_COLON, "Expected ':' before return type in native function type");
    Type *return_type = parser_type(parser);

    Type *func_type = ast_create_function_type(parser->arena, return_type, param_types, param_count);
    func_type->as.function.is_native = true;

    return func_type;
}

/* Helper function to parse a struct method declaration */
static StructMethod *parser_struct_method(Parser *parser, bool is_static, bool is_native_method)
{
    Token fn_token = parser->previous;  /* Points to 'fn' */
    Token method_name;

    if (parser_check(parser, TOKEN_IDENTIFIER))
    {
        method_name = parser->current;
        parser_advance(parser);
        method_name.start = arena_strndup(parser->arena, method_name.start, method_name.length);
        if (method_name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
    }
    else
    {
        parser_error_at_current(parser, "Expected method name");
        return NULL;
    }

    /* Parse parameters */
    Parameter *params = NULL;
    int param_count = 0;
    int param_capacity = 0;
    bool is_variadic = false;

    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        if (!parser_check(parser, TOKEN_RIGHT_PAREN))
        {
            do
            {
                /* Check for variadic parameter */
                if (parser_check(parser, TOKEN_SPREAD))
                {
                    parser_advance(parser);
                    is_variadic = true;
                    break;
                }

                if (param_count >= 255)
                {
                    parser_error_at_current(parser, "Cannot have more than 255 parameters");
                }
                Token param_name;
                if (parser_check(parser, TOKEN_IDENTIFIER))
                {
                    param_name = parser->current;
                    parser_advance(parser);
                    param_name.start = arena_strndup(parser->arena, param_name.start, param_name.length);
                    if (param_name.start == NULL)
                    {
                        parser_error_at_current(parser, "Out of memory");
                        return NULL;
                    }
                }
                else
                {
                    parser_error_at_current(parser, "Expected parameter name");
                    return NULL;
                }
                parser_consume(parser, TOKEN_COLON, "Expected ':' after parameter name");
                SyncModifier param_sync = SYNC_NONE;
                if (parser_match(parser, TOKEN_SYNC))
                {
                    param_sync = SYNC_ATOMIC;
                }
                Type *param_type = parser_type(parser);
                MemoryQualifier param_qualifier = parser_memory_qualifier(parser);

                if (param_count >= param_capacity)
                {
                    param_capacity = param_capacity == 0 ? 8 : param_capacity * 2;
                    Parameter *new_params = arena_alloc(parser->arena, sizeof(Parameter) * param_capacity);
                    if (new_params == NULL)
                    {
                        parser_error_at_current(parser, "Out of memory");
                        return NULL;
                    }
                    if (params != NULL && param_count > 0)
                    {
                        memcpy(new_params, params, sizeof(Parameter) * param_count);
                    }
                    params = new_params;
                }
                params[param_count].name = param_name;
                params[param_count].type = param_type;
                params[param_count].mem_qualifier = param_qualifier;
                params[param_count].sync_modifier = param_sync;
                param_count++;
            } while (parser_match(parser, TOKEN_COMMA));
        }
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");
    }

    /* Parse optional function modifier (shared/private) before return type */
    FunctionModifier func_modifier = parser_function_modifier(parser);

    /* Parse return type */
    Type *return_type = ast_create_primitive_type(parser->arena, TYPE_VOID);
    if (parser_match(parser, TOKEN_COLON))
    {
        return_type = parser_type(parser);
    }

    /* Parse method body if present */
    Stmt **body_stmts = NULL;
    int body_count = 0;

    if (parser_match(parser, TOKEN_ARROW))
    {
        Token arrow_token = parser->previous;

        /* Check for expression-bodied method (expression on same line as arrow) */
        if (parser->current.line == arrow_token.line &&
            parser_can_start_expression(parser->current.type))
        {
            /* Expression-bodied method: fn foo(): int => 42 */
            Expr *body_expr = parser_expression(parser);

            /* Create an implicit return statement wrapping the expression */
            Stmt *return_stmt = ast_create_return_stmt(parser->arena, arrow_token, body_expr, &arrow_token);

            body_stmts = arena_alloc(parser->arena, sizeof(Stmt *));
            if (body_stmts == NULL)
            {
                parser_error(parser, "Out of memory");
                return NULL;
            }
            body_stmts[0] = return_stmt;
            body_count = 1;
        }
        else
        {
            /* Block-bodied method */
            skip_newlines(parser);

            Stmt *body = parser_indented_block(parser);
            if (body != NULL)
            {
                body_stmts = body->as.block.statements;
                body_count = body->as.block.count;
                body->as.block.statements = NULL;
            }
        }
    }
    else if (!is_native_method)
    {
        /* Non-native methods require a body */
        parser_error_at_current(parser, "Expected '=>' before method body");
        return NULL;
    }

    /* Create the method structure */
    StructMethod *method = arena_alloc(parser->arena, sizeof(StructMethod));
    if (method == NULL)
    {
        parser_error_at_current(parser, "Out of memory");
        return NULL;
    }

    method->name = method_name.start;
    method->params = params;
    method->param_count = param_count;
    method->return_type = return_type;
    method->body = body_stmts;
    method->body_count = body_count;
    method->modifier = func_modifier;
    method->is_static = is_static;
    method->is_native = is_native_method;
    method->name_token = method_name;
    method->c_alias = NULL;  /* Set via #pragma alias if needed */

    return method;
}

/* Helper function to check if the current tokens indicate a method declaration */
static bool parser_is_method_start(Parser *parser)
{
    /* Methods can start with: fn, static fn, native fn, static native fn */
    if (parser_check(parser, TOKEN_FN))
    {
        return true;
    }
    if (parser_check(parser, TOKEN_STATIC))
    {
        return true;
    }
    if (parser_check(parser, TOKEN_NATIVE))
    {
        /* In a struct body context, 'native' can only start a method (native fn).
         * We don't peek ahead because parser_advance() modifies lexer state
         * that cannot be easily restored. The actual 'fn' requirement is
         * validated when we try to match TOKEN_FN after consuming 'native'. */
        return true;
    }
    return false;
}

Stmt *parser_struct_declaration(Parser *parser, bool is_native)
{
    Token struct_token = parser->previous;
    Token name;

    /* Parse struct name */
    if (parser_check(parser, TOKEN_IDENTIFIER))
    {
        name = parser->current;
        parser_advance(parser);
        name.start = arena_strndup(parser->arena, name.start, name.length);
        if (name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
    }
    else
    {
        parser_error_at_current(parser, "Expected struct name");
        return NULL;
    }

    /* Parse optional 'as ref' or 'as val' for native structs */
    bool pass_self_by_ref = false;
    if (parser_match(parser, TOKEN_AS))
    {
        if (!is_native)
        {
            parser_error_at_current(parser, "'as ref'/'as val' is only allowed on native structs");
            return NULL;
        }
        if (parser_match(parser, TOKEN_REF))
        {
            pass_self_by_ref = true;
        }
        else if (parser_match(parser, TOKEN_VAL))
        {
            pass_self_by_ref = false;
        }
        else
        {
            parser_error_at_current(parser, "Expected 'ref' or 'val' after 'as'");
            return NULL;
        }
    }

    /* Parse '=>' */
    parser_consume(parser, TOKEN_ARROW, "Expected '=>' after struct name or 'as ref'/'as val'");
    skip_newlines(parser);

    /* Register an incomplete struct type early so method bodies can reference it.
     * This allows static methods to return struct literals like: return Point { x: 0, y: 0 }
     * The type will be updated with complete fields/methods after parsing finishes. */
    Type *early_struct_type = ast_create_struct_type(parser->arena, name.start, NULL, 0,
                                                      NULL, 0, is_native, false, pass_self_by_ref, NULL);
    symbol_table_add_type(parser->symbol_table, name, early_struct_type);

    /* Parse field and method declarations in indented block */
    StructField *fields = NULL;
    int field_count = 0;
    int field_capacity = 0;

    StructMethod *methods = NULL;
    int method_count = 0;
    int method_capacity = 0;

    /* Local pending alias for fields/methods inside struct body */
    const char *member_alias = NULL;

    /* Check for indented block */
    if (parser_check(parser, TOKEN_INDENT))
    {
        parser_advance(parser);

        /* Parse field and method declarations until dedent */
        while (!parser_is_at_end(parser) && !parser_check(parser, TOKEN_DEDENT))
        {
            /* Skip newlines */
            while (parser_match(parser, TOKEN_NEWLINE))
            {
            }

            if (parser_check(parser, TOKEN_DEDENT) || parser_is_at_end(parser))
            {
                break;
            }

            /* Check for #pragma alias inside struct body */
            if (parser_match(parser, TOKEN_PRAGMA_ALIAS))
            {
                if (!parser_match(parser, TOKEN_STRING_LITERAL))
                {
                    parser_error_at_current(parser, "Expected string literal after #pragma alias");
                    continue;
                }
                Token alias_token = parser->previous;
                /* Extract alias string (remove quotes) */
                const char *alias_start = alias_token.start + 1;
                int alias_len = alias_token.length - 2;
                member_alias = arena_strndup(parser->arena, alias_start, alias_len);
                /* Consume newline after pragma */
                parser_match(parser, TOKEN_NEWLINE);
                continue;
            }

            /* Check if this is a method declaration */
            if (parser_is_method_start(parser))
            {
                /* Parse method modifiers */
                bool is_method_static = false;
                bool is_method_native = false;

                /* Check for 'static' modifier */
                if (parser_match(parser, TOKEN_STATIC))
                {
                    is_method_static = true;
                }

                /* Check for 'native' modifier */
                if (parser_match(parser, TOKEN_NATIVE))
                {
                    is_method_native = true;
                }

                /* Now we should be at 'fn' */
                if (!parser_match(parser, TOKEN_FN))
                {
                    parser_error_at_current(parser, "Expected 'fn' keyword");
                    continue;
                }

                /* Parse method */
                StructMethod *method = parser_struct_method(parser, is_method_static, is_method_native);
                if (method == NULL)
                {
                    /* Error already reported */
                    continue;
                }

                /* Check for duplicate method names.
                 * Allow same name if one is static and one is instance method. */
                for (int i = 0; i < method_count; i++)
                {
                    if (strcmp(methods[i].name, method->name) == 0 &&
                        methods[i].is_static == method->is_static)
                    {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "Duplicate %s method name '%s' in struct '%.*s'",
                                 method->is_static ? "static" : "instance",
                                 method->name, name.length, name.start);
                        parser_error_at(parser, &method->name_token, msg);
                        break;
                    }
                }

                /* Grow methods array if needed */
                if (method_count >= method_capacity)
                {
                    method_capacity = method_capacity == 0 ? 4 : method_capacity * 2;
                    StructMethod *new_methods = arena_alloc(parser->arena, sizeof(StructMethod) * method_capacity);
                    if (new_methods == NULL)
                    {
                        parser_error_at_current(parser, "Out of memory");
                        return NULL;
                    }
                    if (methods != NULL && method_count > 0)
                    {
                        memcpy(new_methods, methods, sizeof(StructMethod) * method_count);
                    }
                    methods = new_methods;
                }

                /* Store method */
                methods[method_count] = *method;
                /* Apply pending alias if present */
                if (member_alias != NULL)
                {
                    methods[method_count].c_alias = member_alias;
                    member_alias = NULL;  /* Reset for next member */
                }
                method_count++;
            }
            else
            {
                /* Parse field: name: type */
                if (!parser_check(parser, TOKEN_IDENTIFIER))
                {
                    parser_error_at_current(parser, "Expected field name or method declaration");
                    break;
                }

                Token field_name = parser->current;
                parser_advance(parser);

                parser_consume(parser, TOKEN_COLON, "Expected ':' after field name");

                Type *field_type = parser_type(parser);

                /* Parse optional default value: = expr */
                Expr *default_value = NULL;
                if (parser_match(parser, TOKEN_EQUAL))
                {
                    default_value = parser_expression(parser);
                }

                /* Grow fields array if needed */
                if (field_count >= field_capacity)
                {
                    field_capacity = field_capacity == 0 ? 8 : field_capacity * 2;
                    StructField *new_fields = arena_alloc(parser->arena, sizeof(StructField) * field_capacity);
                    if (new_fields == NULL)
                    {
                        parser_error_at_current(parser, "Out of memory");
                        return NULL;
                    }
                    if (fields != NULL && field_count > 0)
                    {
                        memcpy(new_fields, fields, sizeof(StructField) * field_count);
                    }
                    fields = new_fields;
                }

                /* Store field name */
                char *stored_name = arena_strndup(parser->arena, field_name.start, field_name.length);
                if (stored_name == NULL)
                {
                    parser_error_at_current(parser, "Out of memory");
                    return NULL;
                }

                /* Check for duplicate field names */
                for (int i = 0; i < field_count; i++)
                {
                    if (strcmp(fields[i].name, stored_name) == 0)
                    {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "Duplicate field name '%s' in struct '%.*s'",
                                 stored_name, name.length, name.start);
                        parser_error_at(parser, &field_name, msg);
                        break;
                    }
                }

                /* Check for pointer fields in non-native structs */
                if (!is_native && field_type != NULL && field_type->kind == TYPE_POINTER)
                {
                    char msg[512];
                    snprintf(msg, sizeof(msg),
                             "Pointer field '%s' not allowed in struct '%.*s'. "
                             "Use 'native struct' for structs with pointer fields:\n"
                             "    native struct %.*s =>\n"
                             "        %s: *...",
                             stored_name, name.length, name.start,
                             name.length, name.start, stored_name);
                    parser_error_at(parser, &field_name, msg);
                }

                /* Store field */
                fields[field_count].name = stored_name;
                fields[field_count].type = field_type;
                fields[field_count].offset = 0;  /* Computed during type checking */
                fields[field_count].default_value = default_value;
                /* Apply pending alias if present */
                if (member_alias != NULL)
                {
                    fields[field_count].c_alias = member_alias;
                    member_alias = NULL;  /* Reset for next member */
                }
                else
                {
                    fields[field_count].c_alias = NULL;
                }
                field_count++;

                /* Consume newline after field definition */
                if (!parser_match(parser, TOKEN_NEWLINE) && !parser_check(parser, TOKEN_DEDENT) && !parser_is_at_end(parser))
                {
                    parser_consume(parser, TOKEN_NEWLINE, "Expected newline after field definition");
                }
            }
        }

        /* Consume dedent */
        if (parser_check(parser, TOKEN_DEDENT))
        {
            parser_advance(parser);
        }
    }

    /* Check if this struct should be packed (from #pragma pack(1)) */
    bool is_packed = (parser->pack_alignment == 1);

    /* Consume any pending alias from #pragma alias */
    const char *c_alias = parser->pending_alias;
    parser->pending_alias = NULL;

    /* Validate: c_alias is only allowed on native structs */
    if (c_alias != NULL && !is_native)
    {
        parser_error_at(parser, &struct_token, "#pragma alias is only allowed on native structs");
        return NULL;
    }

    /* Create the struct type for the symbol table */
    Type *struct_type = ast_create_struct_type(parser->arena, name.start, fields, field_count,
                                                methods, method_count, is_native, is_packed,
                                                pass_self_by_ref, c_alias);

    /* Register the struct type in the symbol table so it can be used by later declarations */
    symbol_table_add_type(parser->symbol_table, name, struct_type);

    /* Create struct declaration statement */
    Stmt *stmt = ast_create_struct_decl_stmt(parser->arena, name, fields, field_count,
                                              methods, method_count, is_native, is_packed,
                                              pass_self_by_ref, c_alias, &struct_token);

    return stmt;
}

Stmt *parser_type_declaration(Parser *parser)
{
    Token type_token = parser->previous;
    Token name;

    if (parser_check(parser, TOKEN_IDENTIFIER))
    {
        name = parser->current;
        parser_advance(parser);
        name.start = arena_strndup(parser->arena, name.start, name.length);
        if (name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
    }
    else
    {
        parser_error_at_current(parser, "Expected type alias name");
        return NULL;
    }

    parser_consume(parser, TOKEN_EQUAL, "Expected '=' after type alias name");

    Type *declared_type = NULL;

    /* Check for 'native fn(...)' (native callback type) or 'opaque' */
    if (parser_match(parser, TOKEN_NATIVE))
    {
        /* Parse native function type: native fn(params): return_type */
        declared_type = parser_native_function_type(parser);
        /* Store the typedef name for code generation */
        if (declared_type != NULL)
        {
            /* Create null-terminated string from token */
            char *typedef_name = arena_alloc(parser->arena, name.length + 1);
            strncpy(typedef_name, name.start, name.length);
            typedef_name[name.length] = '\0';
            declared_type->as.function.typedef_name = typedef_name;
        }
    }
    else if (parser_match(parser, TOKEN_OPAQUE))
    {
        /* Create the opaque type with the name */
        declared_type = ast_create_opaque_type(parser->arena, name.start);
    }
    else
    {
        parser_error_at_current(parser, "Expected 'opaque' or 'native fn' after '=' in type declaration");
        return NULL;
    }

    if (declared_type == NULL)
    {
        return NULL;
    }

    /* Register the type in the symbol table immediately so it can be used by later declarations */
    symbol_table_add_type(parser->symbol_table, name, declared_type);

    /* Consume optional newline/semicolon */
    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        if (!parser_check(parser, TOKEN_DEDENT) && !parser_check(parser, TOKEN_FN) &&
            !parser_check(parser, TOKEN_NATIVE) && !parser_check(parser, TOKEN_VAR) &&
            !parser_check(parser, TOKEN_KEYWORD_TYPE))
        {
            parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after type declaration");
        }
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
    }

    return ast_create_type_decl_stmt(parser->arena, name, declared_type, &type_token);
}
