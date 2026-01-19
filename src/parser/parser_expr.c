#include "parser/parser_expr.h"
#include "parser/parser_util.h"
#include "parser/parser_stmt.h"
#include "ast/ast_expr.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

Expr *parser_multi_line_expression(Parser *parser)
{
    Expr *expr = parser_expression(parser);

    while (parser_match(parser, TOKEN_NEWLINE))
    {
        Token op_token = parser->previous;
        Expr *right = parser_expression(parser);
        expr = ast_create_binary_expr(parser->arena, expr, TOKEN_PLUS, right, &op_token);
    }

    return expr;
}

Expr *parser_expression(Parser *parser)
{
    Expr *result = parser_assignment(parser);
    if (result == NULL)
    {
        parser_error_at_current(parser, "Expected expression");
        parser_advance(parser);
    }
    return result;
}

Expr *parser_assignment(Parser *parser)
{
    Expr *expr = parser_logical_or(parser);

    if (parser_match(parser, TOKEN_EQUAL))
    {
        Token equals = parser->previous;
        Expr *value = parser_assignment(parser);
        if (expr->type == EXPR_VARIABLE)
        {
            Token name = expr->as.variable.name;
            char *new_start = arena_strndup(parser->arena, name.start, name.length);
            if (new_start == NULL)
            {
                exit(1);
            }
            name.start = new_start;
            return ast_create_assign_expr(parser->arena, name, value, &equals);
        }
        else if (expr->type == EXPR_ARRAY_ACCESS)
        {
            return ast_create_index_assign_expr(parser->arena,
                expr->as.array_access.array,
                expr->as.array_access.index,
                value, &equals);
        }
        else if (expr->type == EXPR_MEMBER)
        {
            /* Struct field assignment: point.x = value */
            return ast_create_member_assign_expr(parser->arena,
                expr->as.member.object,
                expr->as.member.member_name,
                value, &equals);
        }
        parser_error(parser, "Invalid assignment target");
    }

    /* Check for compound assignment: +=, -=, *=, /=, %= */
    SnTokenType compound_op = TOKEN_EOF;
    if (parser_match(parser, TOKEN_PLUS_EQUAL))
    {
        compound_op = TOKEN_PLUS;
    }
    else if (parser_match(parser, TOKEN_MINUS_EQUAL))
    {
        compound_op = TOKEN_MINUS;
    }
    else if (parser_match(parser, TOKEN_STAR_EQUAL))
    {
        compound_op = TOKEN_STAR;
    }
    else if (parser_match(parser, TOKEN_SLASH_EQUAL))
    {
        compound_op = TOKEN_SLASH;
    }
    else if (parser_match(parser, TOKEN_MODULO_EQUAL))
    {
        compound_op = TOKEN_MODULO;
    }

    if (compound_op != TOKEN_EOF)
    {
        Token op_token = parser->previous;
        Expr *value = parser_assignment(parser);
        return ast_create_compound_assign_expr(parser->arena, expr, compound_op, value, &op_token);
    }

    return expr;
}

Expr *parser_logical_or(Parser *parser)
{
    Expr *expr = parser_logical_and(parser);
    while (parser_match(parser, TOKEN_OR))
    {
        Token op = parser->previous;
        SnTokenType operator = op.type;
        Expr *right = parser_logical_and(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
    }
    return expr;
}

Expr *parser_logical_and(Parser *parser)
{
    Expr *expr = parser_equality(parser);
    while (parser_match(parser, TOKEN_AND))
    {
        Token op = parser->previous;
        SnTokenType operator = op.type;
        Expr *right = parser_equality(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
    }
    return expr;
}

Expr *parser_equality(Parser *parser)
{
    Expr *expr = parser_comparison(parser);
    while (parser_match(parser, TOKEN_BANG_EQUAL) || parser_match(parser, TOKEN_EQUAL_EQUAL))
    {
        Token op = parser->previous;
        SnTokenType operator = op.type;
        Expr *right = parser_comparison(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
    }
    return expr;
}

Expr *parser_comparison(Parser *parser)
{
    Expr *expr = parser_range(parser);
    while (parser_match(parser, TOKEN_LESS) || parser_match(parser, TOKEN_LESS_EQUAL) ||
           parser_match(parser, TOKEN_GREATER) || parser_match(parser, TOKEN_GREATER_EQUAL))
    {
        Token op = parser->previous;
        SnTokenType operator = op.type;
        Expr *right = parser_range(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
    }
    return expr;
}

Expr *parser_range(Parser *parser)
{
    Expr *expr = parser_term(parser);

    // Check for range operator: expr..end
    if (parser_match(parser, TOKEN_RANGE))
    {
        Token range_token = parser->previous;
        Expr *end = parser_term(parser);
        return ast_create_range_expr(parser->arena, expr, end, &range_token);
    }

    return expr;
}

Expr *parser_term(Parser *parser)
{
    Expr *expr = parser_factor(parser);
    while (parser_match(parser, TOKEN_PLUS) || parser_match(parser, TOKEN_MINUS))
    {
        Token op = parser->previous;
        SnTokenType operator = op.type;
        Expr *right = parser_factor(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
    }
    return expr;
}

Expr *parser_factor(Parser *parser)
{
    Expr *expr = parser_unary(parser);
    while (parser_match(parser, TOKEN_STAR) || parser_match(parser, TOKEN_SLASH) || parser_match(parser, TOKEN_MODULO))
    {
        Token op = parser->previous;
        SnTokenType operator = op.type;
        Expr *right = parser_unary(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
    }
    return expr;
}

Expr *parser_unary(Parser *parser)
{
    if (parser_match(parser, TOKEN_BANG) || parser_match(parser, TOKEN_MINUS))
    {
        Token op = parser->previous;
        SnTokenType operator = op.type;
        Expr *right = parser_unary(parser);
        return ast_create_unary_expr(parser->arena, operator, right, &op);
    }
    /* typeof operator: typeof expr or typeof(expr) or typeof int or typeof(int) */
    if (parser_match(parser, TOKEN_TYPEOF))
    {
        Token typeof_token = parser->previous;
        bool has_parens = parser_match(parser, TOKEN_LEFT_PAREN);

        /* Check if this is typeof <type> (e.g., typeof int, typeof str) */
        /* We need to check for type keywords first */
        if (parser_check(parser, TOKEN_INT) || parser_check(parser, TOKEN_INT32) ||
            parser_check(parser, TOKEN_UINT) || parser_check(parser, TOKEN_UINT32) ||
            parser_check(parser, TOKEN_LONG) || parser_check(parser, TOKEN_DOUBLE) ||
            parser_check(parser, TOKEN_FLOAT) || parser_check(parser, TOKEN_CHAR) ||
            parser_check(parser, TOKEN_STR) || parser_check(parser, TOKEN_BOOL) ||
            parser_check(parser, TOKEN_BYTE) || parser_check(parser, TOKEN_VOID) ||
            parser_check(parser, TOKEN_ANY))
        {
            Type *type_literal = parser_type(parser);
            if (has_parens)
            {
                parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after typeof type");
            }
            return ast_create_typeof_expr(parser->arena, NULL, type_literal, &typeof_token);
        }
        else
        {
            /* typeof expression */
            Expr *operand = parser_unary(parser);
            if (has_parens)
            {
                parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after typeof expression");
            }
            return ast_create_typeof_expr(parser->arena, operand, NULL, &typeof_token);
        }
    }
    /* sizeof operator: sizeof expr or sizeof(expr) or sizeof Type or sizeof(Type) */
    if (parser_match(parser, TOKEN_SIZEOF))
    {
        Token sizeof_token = parser->previous;
        bool has_parens = parser_match(parser, TOKEN_LEFT_PAREN);

        /* Check if this is sizeof <type> (e.g., sizeof Point, sizeof int)
         * We need to check for type keywords and struct names first */
        if (parser_check(parser, TOKEN_INT) || parser_check(parser, TOKEN_INT32) ||
            parser_check(parser, TOKEN_UINT) || parser_check(parser, TOKEN_UINT32) ||
            parser_check(parser, TOKEN_LONG) || parser_check(parser, TOKEN_DOUBLE) ||
            parser_check(parser, TOKEN_FLOAT) || parser_check(parser, TOKEN_CHAR) ||
            parser_check(parser, TOKEN_STR) || parser_check(parser, TOKEN_BOOL) ||
            parser_check(parser, TOKEN_BYTE) || parser_check(parser, TOKEN_VOID) ||
            parser_check(parser, TOKEN_ANY) ||
            parser_check(parser, TOKEN_STAR))
        {
            /* This is sizeof on a type - parse as type */
            Type *type_operand = parser_type(parser);
            if (has_parens)
            {
                parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after sizeof type");
            }
            return ast_create_sizeof_type_expr(parser->arena, type_operand, &sizeof_token);
        }
        else if (parser_check(parser, TOKEN_IDENTIFIER))
        {
            /* Could be either a type name (struct) or an expression (variable)
             * If followed by { it's a struct literal expression
             * Otherwise, check if it's a known struct type or fall back to expression */
            Token id = parser->current;

            /* Peek ahead to see what follows the identifier */
            /* For sizeof, if it's a struct type name, parse as type; otherwise as expression */
            /* Use the symbol table to check if it's a struct type */
            Symbol *type_symbol = symbol_table_lookup_type(parser->symbol_table, id);
            if (type_symbol != NULL && type_symbol->type != NULL &&
                type_symbol->type->kind == TYPE_STRUCT)
            {
                /* It's a struct type name */
                Type *type_operand = parser_type(parser);
                if (has_parens)
                {
                    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after sizeof type");
                }
                return ast_create_sizeof_type_expr(parser->arena, type_operand, &sizeof_token);
            }
            else
            {
                /* Parse as expression */
                Expr *operand = parser_unary(parser);
                if (has_parens)
                {
                    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after sizeof expression");
                }
                return ast_create_sizeof_expr_expr(parser->arena, operand, &sizeof_token);
            }
        }
        else
        {
            /* sizeof expression */
            Expr *operand = parser_unary(parser);
            if (has_parens)
            {
                parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after sizeof expression");
            }
            return ast_create_sizeof_expr_expr(parser->arena, operand, &sizeof_token);
        }
    }
    /* Thread spawn: &fn() or &fn()!
     * We need to parse only the call expression without postfix operators,
     * because parser_postfix() would consume the ! and turn it into a sync
     * before we can create the spawn expression. */
    if (parser_match(parser, TOKEN_AMPERSAND))
    {
        Token ampersand = parser->previous;
        /* Parse primary expression (function name or Type for static call) */
        Expr *call_expr = parser_primary(parser);
        if (call_expr == NULL)
        {
            parser_error(parser, "Expected function call after '&'");
            return NULL;
        }
        /* Now handle only call-related postfix operations (not !) */
        for (;;)
        {
            if (parser_match(parser, TOKEN_LEFT_PAREN))
            {
                call_expr = parser_call(parser, call_expr);
            }
            else if (parser_match(parser, TOKEN_LEFT_BRACKET))
            {
                call_expr = parser_array_access(parser, call_expr);
            }
            else if (parser_match(parser, TOKEN_DOT))
            {
                Token dot = parser->previous;
                if (!parser_check_method_name(parser))
                {
                    parser_error_at_current(parser, "Expected identifier after '.'");
                }
                Token member_name = parser->current;
                parser_advance(parser);
                call_expr = ast_create_member_expr(parser->arena, call_expr, member_name, &dot);
            }
            else
            {
                break;
            }
        }
        /* Verify it's a call expression */
        if (call_expr->type != EXPR_CALL && call_expr->type != EXPR_STATIC_CALL)
        {
            parser_error(parser, "Thread spawn '&' requires a function call");
            return NULL;
        }
        /* Create spawn with FUNC_DEFAULT; actual modifier determined during type checking */
        Expr *spawn = ast_create_thread_spawn_expr(parser->arena, call_expr, FUNC_DEFAULT, &ampersand);
        /* Check for immediate sync: &fn()! */
        if (parser_match(parser, TOKEN_BANG))
        {
            Token bang = parser->previous;
            return ast_create_thread_sync_expr(parser->arena, spawn, false, &bang);
        }
        return spawn;
    }
    return parser_postfix(parser);
}

Expr *parser_postfix(Parser *parser)
{
    Expr *expr = parser_primary(parser);
    for (;;)
    {
        if (parser_match(parser, TOKEN_LEFT_PAREN))
        {
            expr = parser_call(parser, expr);
        }
        else if (parser_match(parser, TOKEN_LEFT_BRACKET))
        {
            expr = parser_array_access(parser, expr);
        }
        else if (parser_match(parser, TOKEN_DOT))
        {
            Token dot = parser->previous;
            if (!parser_check_method_name(parser))
            {
                parser_error_at_current(parser, "Expected identifier after '.'");
            }
            Token member_name = parser->current;
            parser_advance(parser);
            expr = ast_create_member_expr(parser->arena, expr, member_name, &dot);
        }
        else if (parser_match(parser, TOKEN_PLUS_PLUS))
        {
            expr = ast_create_increment_expr(parser->arena, expr, &parser->previous);
        }
        else if (parser_match(parser, TOKEN_MINUS_MINUS))
        {
            expr = ast_create_decrement_expr(parser->arena, expr, &parser->previous);
        }
        else if (parser_match(parser, TOKEN_BANG))
        {
            /* Thread sync: r! for single, [r1, r2, r3]! for multiple */
            Token bang = parser->previous;
            bool is_sync_list = (expr->type == EXPR_SYNC_LIST);
            expr = ast_create_thread_sync_expr(parser->arena, expr, is_sync_list, &bang);
        }
        else if (parser_match(parser, TOKEN_AS))
        {
            /* "as val" postfix operator: expr as val - dereferences pointer to value */
            /* "as <type>" postfix operator: expr as int - casts any to concrete type */
            Token as_token = parser->previous;
            if (parser_match(parser, TOKEN_VAL))
            {
                expr = ast_create_as_val_expr(parser->arena, expr, &as_token);
            }
            else if (parser_match(parser, TOKEN_REF))
            {
                /* "as ref" postfix operator: expr as ref - gets pointer to value */
                expr = ast_create_as_ref_expr(parser->arena, expr, &as_token);
            }
            else
            {
                /* Parse a type for type casting */
                Type *target_type = parser_type(parser);
                if (target_type == NULL)
                {
                    parser_error_at_current(parser, "Expected type after 'as'");
                }
                else
                {
                    expr = ast_create_as_type_expr(parser->arena, expr, target_type, &as_token);
                }
            }
        }
        else if (parser_match(parser, TOKEN_IS))
        {
            /* "is <type>" postfix operator: expr is int - checks if any value is of type */
            Token is_token = parser->previous;
            Type *check_type = parser_type(parser);
            if (check_type == NULL)
            {
                parser_error_at_current(parser, "Expected type after 'is'");
            }
            else
            {
                expr = ast_create_is_expr(parser->arena, expr, check_type, &is_token);
            }
        }
        else
        {
            break;
        }
    }
    return expr;
}

Expr *parser_primary(Parser *parser)
{
    if (parser_match(parser, TOKEN_INT_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_INT), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_LONG_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_LONG), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_BYTE_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_BYTE), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_DOUBLE_LITERAL))
    {
        LiteralValue value;
        value.double_value = parser->previous.literal.double_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_DOUBLE), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_FLOAT_LITERAL))
    {
        LiteralValue value;
        value.double_value = parser->previous.literal.double_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_FLOAT), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_UINT_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_UINT), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_UINT32_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_UINT32), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_INT32_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_INT32), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_CHAR_LITERAL))
    {
        LiteralValue value;
        value.char_value = parser->previous.literal.char_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_CHAR), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_STRING_LITERAL))
    {
        // Handle error case where string literal content is NULL (e.g., unterminated string)
        if (parser->previous.literal.string_value == NULL)
        {
            parser_error(parser, "Invalid string literal");
            return NULL;
        }
        LiteralValue value;
        value.string_value = arena_strdup(parser->arena, parser->previous.literal.string_value);
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_STRING), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_BOOL_LITERAL))
    {
        LiteralValue value;
        value.bool_value = parser->previous.literal.bool_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_BOOL), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_NIL))
    {
        LiteralValue value;
        value.int_value = 0;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_NIL), false, &parser->previous);
    }
    /* Lambda expression: fn(x: int, y: int) shared: int => x + y
     * Or with type inference: fn(x, y) => x + y (types inferred from context) */
    if (parser_match(parser, TOKEN_FN))
    {
        Token fn_token = parser->previous;
        parser_consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'fn' in lambda");

        Parameter *params = NULL;
        int param_count = 0;
        int param_capacity = 0;

        if (!parser_check(parser, TOKEN_RIGHT_PAREN))
        {
            do
            {
                /* Parse: name [: type] [as val|ref] */
                Token param_name = parser->current;
                parser_consume(parser, TOKEN_IDENTIFIER, "Expected parameter name");
                param_name.start = arena_strndup(parser->arena, param_name.start, param_name.length);

                /* Type is optional - if no colon, set type to NULL for inference */
                Type *param_type = NULL;
                if (parser_match(parser, TOKEN_COLON))
                {
                    param_type = parser_type(parser);
                }

                /* Parse optional "as val" or "as ref" for parameter */
                MemoryQualifier param_qualifier = parser_memory_qualifier(parser);

                /* Add to params array */
                if (param_count >= param_capacity)
                {
                    param_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
                    Parameter *new_params = arena_alloc(parser->arena, sizeof(Parameter) * param_capacity);
                    if (new_params == NULL)
                    {
                        parser_error(parser, "Out of memory");
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
                param_count++;
            } while (parser_match(parser, TOKEN_COMMA));
        }

        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after lambda parameters");

        /* Parse optional function modifier (shared/private) before return type */
        FunctionModifier modifier = parser_function_modifier(parser);

        /* Return type is optional - if => comes next, set return_type to NULL for inference */
        Type *return_type = NULL;
        if (parser_match(parser, TOKEN_COLON))
        {
            return_type = parser_type(parser);
        }

        /* Parse body */
        parser_consume(parser, TOKEN_ARROW, "Expected '=>' before lambda body");

        /* Check for multi-line lambda body (newline followed by indent) */
        if (parser_check(parser, TOKEN_NEWLINE))
        {
            skip_newlines(parser);
            if (parser_check(parser, TOKEN_INDENT))
            {
                /* Multi-line lambda with statement body */
                Stmt *block = parser_indented_block(parser);
                if (block == NULL)
                {
                    parser_error(parser, "Expected indented block for lambda body");
                    return NULL;
                }
                Stmt **stmts = block->as.block.statements;
                int stmt_count = block->as.block.count;
                bool is_native_lambda = parser->in_native_function != 0;
                return ast_create_lambda_stmt_expr(parser->arena, params, param_count,
                                                   return_type, stmts, stmt_count, modifier,
                                                   is_native_lambda, &fn_token);
            }
            /* Newline but no indent - single expression on next line is not valid */
            parser_error(parser, "Expected expression or indented block after '=>'");
            return NULL;
        }

        /* Single-line lambda with expression body */
        Expr *body = parser_expression(parser);
        bool is_native_lambda = parser->in_native_function != 0;
        return ast_create_lambda_expr(parser->arena, params, param_count, return_type, body,
                                      modifier, is_native_lambda, &fn_token);
    }
    if (parser_match(parser, TOKEN_IDENTIFIER))
    {
        Token var_token = parser->previous;

        /* Check for struct literal: TypeName { field: value, ... } or TypeName {} */
        if (parser_check(parser, TOKEN_LEFT_BRACE))
        {
            /* Check if this identifier is a known struct type */
            Symbol *type_symbol = symbol_table_lookup_type(parser->symbol_table, var_token);
            if (type_symbol != NULL && type_symbol->type != NULL &&
                type_symbol->type->kind == TYPE_STRUCT)
            {
                /* This is a struct literal */
                Token struct_name = var_token;
                struct_name.start = arena_strndup(parser->arena, var_token.start, var_token.length);
                if (struct_name.start == NULL)
                {
                    parser_error(parser, "Out of memory");
                    return NULL;
                }

                parser_advance(parser);  /* consume '{' */
                Token left_brace = parser->previous;

                FieldInitializer *fields = NULL;
                int field_count = 0;
                int field_capacity = 0;

                /* Skip newlines/indent/dedent after opening brace for multi-line struct literals */
                while (parser_match(parser, TOKEN_NEWLINE) ||
                       parser_match(parser, TOKEN_INDENT) ||
                       parser_match(parser, TOKEN_DEDENT)) {}

                if (!parser_check(parser, TOKEN_RIGHT_BRACE))
                {
                    do
                    {
                        /* Skip newlines/indent/dedent after comma for multi-line struct literals */
                        while (parser_match(parser, TOKEN_NEWLINE) ||
                               parser_match(parser, TOKEN_INDENT) ||
                               parser_match(parser, TOKEN_DEDENT)) {}

                        /* Parse field name */
                        if (!parser_check(parser, TOKEN_IDENTIFIER))
                        {
                            parser_error_at_current(parser, "Expected field name in struct literal");
                            break;
                        }
                        Token field_name = parser->current;
                        parser_advance(parser);

                        /* Parse colon */
                        parser_consume(parser, TOKEN_COLON, "Expected ':' after field name");

                        /* Parse field value */
                        Expr *field_value = parser_expression(parser);
                        if (field_value == NULL)
                        {
                            parser_error(parser, "Expected field value");
                            break;
                        }

                        /* Add to fields array */
                        if (field_count >= field_capacity)
                        {
                            field_capacity = field_capacity == 0 ? 4 : field_capacity * 2;
                            FieldInitializer *new_fields = arena_alloc(parser->arena,
                                sizeof(FieldInitializer) * field_capacity);
                            if (new_fields == NULL)
                            {
                                parser_error(parser, "Out of memory");
                                return NULL;
                            }
                            if (fields != NULL && field_count > 0)
                            {
                                memcpy(new_fields, fields, sizeof(FieldInitializer) * field_count);
                            }
                            fields = new_fields;
                        }
                        fields[field_count].name = field_name;
                        fields[field_count].value = field_value;
                        field_count++;
                    } while (parser_match(parser, TOKEN_COMMA));
                }

                /* Skip newlines/indent/dedent before closing brace for multi-line struct literals */
                while (parser_match(parser, TOKEN_NEWLINE) ||
                       parser_match(parser, TOKEN_INDENT) ||
                       parser_match(parser, TOKEN_DEDENT)) {}

                parser_consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after struct literal");

                return ast_create_struct_literal_expr(parser->arena, struct_name, fields,
                                                       field_count, &left_brace);
            }
        }

        /* Check for static method call: TypeName.method() */
        /* Recognize both built-in types and user-defined struct types */
        bool is_static_type = parser_is_static_type_name(var_token.start, var_token.length);
        if (!is_static_type && parser_check(parser, TOKEN_DOT))
        {
            /* Check if it's a user-defined struct type */
            Symbol *type_symbol = symbol_table_lookup_type(parser->symbol_table, var_token);
            if (type_symbol != NULL && type_symbol->type != NULL &&
                type_symbol->type->kind == TYPE_STRUCT)
            {
                is_static_type = true;
            }
        }
        if (parser_check(parser, TOKEN_DOT) && is_static_type)
        {
            /* Save the type name token */
            Token type_name = var_token;
            type_name.start = arena_strndup(parser->arena, var_token.start, var_token.length);

            /* Consume the dot */
            parser_advance(parser);

            /* Expect method name (identifier or type keyword like int, long, double, etc.) */
            if (!parser_check_method_name(parser))
            {
                parser_error_at_current(parser, "Expected method name after '.'");
                return NULL;
            }
            Token method_name = parser->current;
            method_name.start = arena_strndup(parser->arena, parser->current.start, parser->current.length);
            parser_advance(parser);

            /* Check for opening paren - must be a call */
            if (!parser_check(parser, TOKEN_LEFT_PAREN))
            {
                parser_error_at_current(parser, "Expected '(' after static method name");
                return NULL;
            }

            /* Parse arguments */
            parser_advance(parser);  /* consume '(' */

            Expr **arguments = NULL;
            int arg_count = 0;
            int arg_capacity = 0;

            if (!parser_check(parser, TOKEN_RIGHT_PAREN))
            {
                do
                {
                    Expr *arg = parser_expression(parser);
                    if (arg_count >= arg_capacity)
                    {
                        arg_capacity = arg_capacity == 0 ? 4 : arg_capacity * 2;
                        Expr **new_args = arena_alloc(parser->arena, sizeof(Expr *) * arg_capacity);
                        if (new_args == NULL)
                        {
                            parser_error(parser, "Out of memory");
                            return NULL;
                        }
                        if (arguments != NULL && arg_count > 0)
                        {
                            memcpy(new_args, arguments, sizeof(Expr *) * arg_count);
                        }
                        arguments = new_args;
                    }
                    arguments[arg_count++] = arg;
                } while (parser_match(parser, TOKEN_COMMA));
            }

            parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after arguments");

            return ast_create_static_call_expr(parser->arena, type_name, method_name,
                                               arguments, arg_count, &type_name);
        }

        var_token.start = arena_strndup(parser->arena, parser->previous.start, parser->previous.length);
        return ast_create_variable_expr(parser->arena, var_token, &parser->previous);
    }
    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        Expr *expr = parser_expression(parser);
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }

    if (parser_match(parser, TOKEN_LEFT_BRACE))
    {
        Token left_brace = parser->previous;
        Expr **elements = NULL;
        int count = 0;
        int capacity = 0;

        /* Skip newlines/indent/dedent after opening brace for multi-line array literals */
        while (parser_match(parser, TOKEN_NEWLINE) ||
               parser_match(parser, TOKEN_INDENT) ||
               parser_match(parser, TOKEN_DEDENT)) {}

        if (!parser_check(parser, TOKEN_RIGHT_BRACE))
        {
            do
            {
                /* Skip newlines/indent/dedent after comma for multi-line array literals */
                while (parser_match(parser, TOKEN_NEWLINE) ||
                       parser_match(parser, TOKEN_INDENT) ||
                       parser_match(parser, TOKEN_DEDENT)) {}

                Expr *elem;
                // Check for spread operator: ...expr
                if (parser_match(parser, TOKEN_SPREAD))
                {
                    Token spread_token = parser->previous;
                    Expr *spread_array = parser_expression(parser);
                    elem = ast_create_spread_expr(parser->arena, spread_array, &spread_token);
                }
                else
                {
                    elem = parser_expression(parser);
                }

                if (elem != NULL)
                {
                    if (count >= capacity)
                    {
                        capacity = capacity == 0 ? 8 : capacity * 2;
                        Expr **new_elements = arena_alloc(parser->arena, sizeof(Expr *) * capacity);
                        if (new_elements == NULL)
                        {
                            parser_error(parser, "Out of memory");
                            return NULL;
                        }
                        if (elements != NULL && count > 0)
                        {
                            memcpy(new_elements, elements, sizeof(Expr *) * count);
                        }
                        elements = new_elements;
                    }
                    elements[count++] = elem;
                }
            } while (parser_match(parser, TOKEN_COMMA));
        }

        /* Skip newlines/indent/dedent before closing brace for multi-line array literals */
        while (parser_match(parser, TOKEN_NEWLINE) ||
               parser_match(parser, TOKEN_INDENT) ||
               parser_match(parser, TOKEN_DEDENT)) {}

        parser_consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after array elements");
        return ast_create_array_expr(parser->arena, elements, count, &left_brace);
    }

    /* Sync list: [r1, r2, r3] - for multi-thread synchronization with ! */
    if (parser_match(parser, TOKEN_LEFT_BRACKET))
    {
        Token left_bracket = parser->previous;
        Expr **elements = NULL;
        int count = 0;
        int capacity = 0;

        if (!parser_check(parser, TOKEN_RIGHT_BRACKET))
        {
            do
            {
                Expr *elem = parser_expression(parser);
                if (elem != NULL)
                {
                    if (count >= capacity)
                    {
                        capacity = capacity == 0 ? 8 : capacity * 2;
                        Expr **new_elements = arena_alloc(parser->arena, sizeof(Expr *) * capacity);
                        if (new_elements == NULL)
                        {
                            parser_error(parser, "Out of memory");
                            return NULL;
                        }
                        if (elements != NULL && count > 0)
                        {
                            memcpy(new_elements, elements, sizeof(Expr *) * count);
                        }
                        elements = new_elements;
                    }
                    elements[count++] = elem;
                }
            } while (parser_match(parser, TOKEN_COMMA));
        }

        parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after sync list elements");
        return ast_create_sync_list_expr(parser->arena, elements, count, &left_bracket);
    }

    if (parser_match(parser, TOKEN_INTERPOL_STRING))
    {
        Token interpol_token = parser->previous;
        const char *content = parser->previous.literal.string_value;

        // Handle error case where string literal content is NULL (e.g., unterminated string)
        if (content == NULL)
        {
            parser_error(parser, "Invalid interpolated string");
            return NULL;
        }

        Expr **parts = NULL;
        char **format_specs = NULL;  // Format specifiers for each part
        int capacity = 0;
        int count = 0;

        // Helper to add a string segment to parts (no format spec for literals)
        #define ADD_STRING_PART(str) do { \
            LiteralValue v; \
            v.string_value = str; \
            Expr *seg_expr = ast_create_literal_expr(parser->arena, v, ast_create_primitive_type(parser->arena, TYPE_STRING), false, &interpol_token); \
            if (count >= capacity) { \
                capacity = capacity == 0 ? 8 : capacity * 2; \
                Expr **new_parts = arena_alloc(parser->arena, sizeof(Expr *) * capacity); \
                char **new_formats = arena_alloc(parser->arena, sizeof(char *) * capacity); \
                if (new_parts == NULL || new_formats == NULL) exit(1); \
                if (parts != NULL && count > 0) { \
                    memcpy(new_parts, parts, sizeof(Expr *) * count); \
                    memcpy(new_formats, format_specs, sizeof(char *) * count); \
                } \
                parts = new_parts; \
                format_specs = new_formats; \
            } \
            parts[count] = seg_expr; \
            format_specs[count] = NULL; \
            count++; \
        } while(0)

        // Helper to add an expression with optional format spec
        #define ADD_EXPR_PART(expr, fmt) do { \
            if (count >= capacity) { \
                capacity = capacity == 0 ? 8 : capacity * 2; \
                Expr **new_parts = arena_alloc(parser->arena, sizeof(Expr *) * capacity); \
                char **new_formats = arena_alloc(parser->arena, sizeof(char *) * capacity); \
                if (new_parts == NULL || new_formats == NULL) exit(1); \
                if (parts != NULL && count > 0) { \
                    memcpy(new_parts, parts, sizeof(Expr *) * count); \
                    memcpy(new_formats, format_specs, sizeof(char *) * count); \
                } \
                parts = new_parts; \
                format_specs = new_formats; \
            } \
            parts[count] = expr; \
            format_specs[count] = fmt; \
            count++; \
        } while(0)

        const char *p = content;
        // Buffer for building text segments (handles escape sequences)
        char *seg_buf = arena_alloc(parser->arena, strlen(content) + 1);
        int seg_len = 0;

        while (*p)
        {
            // Handle {{ escape sequence -> literal {
            if (*p == '{' && *(p + 1) == '{')
            {
                seg_buf[seg_len++] = '{';
                p += 2;
                continue;
            }
            // Handle }} escape sequence -> literal }
            if (*p == '}' && *(p + 1) == '}')
            {
                seg_buf[seg_len++] = '}';
                p += 2;
                continue;
            }
            // Handle interpolation expression {expr} or {expr:format}
            if (*p == '{')
            {
                // Flush accumulated text segment
                if (seg_len > 0)
                {
                    seg_buf[seg_len] = '\0';
                    char *seg = arena_strdup(parser->arena, seg_buf);
                    ADD_STRING_PART(seg);
                    seg_len = 0;
                }

                p++; // skip {
                const char *expr_start = p;
                const char *colon_pos = NULL;  // Position of format specifier colon

                // Use a state stack to track nesting
                // State types: 0 = in code (top level), 1 = in regular string,
                //              2 = in interp string text, 3 = in code within interp string
                #define MAX_NESTING 64
                int state_stack[MAX_NESTING];
                int stack_top = 0;
                state_stack[stack_top] = 0;  // Start in code mode (within our brace)

                while (*p && stack_top >= 0)
                {
                    int state = state_stack[stack_top];

                    if (state == 1)  // In regular string
                    {
                        if (*p == '\\' && *(p + 1))
                        {
                            p += 2;
                            continue;
                        }
                        if (*p == '"')
                        {
                            stack_top--;  // Exit regular string
                            p++;
                            continue;
                        }
                        p++;
                        continue;
                    }

                    if (state == 2)  // In interpolated string text
                    {
                        if (*p == '\\' && *(p + 1))
                        {
                            p += 2;
                            continue;
                        }
                        if (*p == '"')
                        {
                            stack_top--;  // Exit interp string
                            p++;
                            continue;
                        }
                        if (*p == '{')
                        {
                            // Entering brace expression within interp string
                            // Change current state to "code within interp"
                            state_stack[stack_top] = 3;
                            p++;
                            continue;
                        }
                        p++;
                        continue;
                    }

                    // state == 0 or 3: In code (0 = top level, 3 = within interp string)
                    if (*p == '$' && *(p + 1) == '"')
                    {
                        // Start nested interpolated string
                        if (stack_top < MAX_NESTING - 1)
                        {
                            stack_top++;
                            state_stack[stack_top] = 2;  // interp string text
                        }
                        p += 2;
                        continue;
                    }
                    if (*p == '"')
                    {
                        // Start regular string
                        if (stack_top < MAX_NESTING - 1)
                        {
                            stack_top++;
                            state_stack[stack_top] = 1;  // regular string
                        }
                        p++;
                        continue;
                    }
                    if (*p == '{')
                    {
                        // Nested brace in code
                        if (stack_top < MAX_NESTING - 1)
                        {
                            stack_top++;
                            state_stack[stack_top] = state;  // preserve whether we're in interp (3) or top level (0)
                        }
                        p++;
                        continue;
                    }
                    if (*p == '}')
                    {
                        if (stack_top == 0)
                        {
                            // This closes our original brace
                            break;
                        }
                        // Pop the state and check if we should return to interp text
                        if (state_stack[stack_top] == 3)
                        {
                            // We were in code within an interp string, return to text mode
                            state_stack[stack_top] = 2;
                        }
                        else
                        {
                            // We were in nested code braces, pop the stack
                            stack_top--;
                        }
                        p++;
                        continue;
                    }
                    if (*p == ':' && stack_top == 0 && colon_pos == NULL)
                    {
                        // Track colon for format specifier (only at our expression level)
                        colon_pos = p;
                    }
                    p++;
                }
                #undef MAX_NESTING
                if (!*p && stack_top > 0)
                {
                    parser_error_at_current(parser, "Unterminated interpolated expression");
                    LiteralValue zero = {0};
                    return ast_create_literal_expr(parser->arena, zero, ast_create_primitive_type(parser->arena, TYPE_STRING), false, NULL);
                }

                // Extract expression and optional format specifier
                char *expr_src;
                char *format_spec = NULL;

                if (colon_pos != NULL)
                {
                    // We have a format specifier: {expr:format}
                    int expr_len = colon_pos - expr_start;
                    expr_src = arena_strndup(parser->arena, expr_start, expr_len);

                    int format_len = p - (colon_pos + 1);
                    if (format_len > 0)
                    {
                        format_spec = arena_strndup(parser->arena, colon_pos + 1, format_len);
                    }
                }
                else
                {
                    // No format specifier: {expr}
                    int expr_len = p - expr_start;
                    expr_src = arena_strndup(parser->arena, expr_start, expr_len);
                }

                Lexer sub_lexer;
                lexer_init(parser->arena, &sub_lexer, expr_src, "interpolated");
                Parser sub_parser;
                parser_init(parser->arena, &sub_parser, &sub_lexer, parser->symbol_table);
                sub_parser.symbol_table = parser->symbol_table;

                Expr *inner = parser_expression(&sub_parser);
                if (inner == NULL || sub_parser.had_error)
                {
                    parser_error_at_current(parser, "Invalid expression in interpolation");
                    LiteralValue zero = {0};
                    return ast_create_literal_expr(parser->arena, zero, ast_create_primitive_type(parser->arena, TYPE_STRING), false, NULL);
                }

                ADD_EXPR_PART(inner, format_spec);

                if (parser->interp_count >= parser->interp_capacity)
                {
                    parser->interp_capacity = parser->interp_capacity ? parser->interp_capacity * 2 : 8;
                    char **new_interp_sources = arena_alloc(parser->arena, sizeof(char *) * parser->interp_capacity);
                    if (new_interp_sources == NULL) exit(1);
                    if (parser->interp_sources != NULL && parser->interp_count > 0)
                        memcpy(new_interp_sources, parser->interp_sources, sizeof(char *) * parser->interp_count);
                    parser->interp_sources = new_interp_sources;
                }
                parser->interp_sources[parser->interp_count++] = expr_src;

                p++; // skip }
            }
            else
            {
                seg_buf[seg_len++] = *p++;
            }
        }

        // Flush any remaining text segment
        if (seg_len > 0)
        {
            seg_buf[seg_len] = '\0';
            char *seg = arena_strdup(parser->arena, seg_buf);
            ADD_STRING_PART(seg);
        }

        #undef ADD_STRING_PART
        #undef ADD_EXPR_PART

        return ast_create_interpolated_expr(parser->arena, parts, format_specs, count, &interpol_token);
    }

    parser_error_at_current(parser, "Expected expression");
    LiteralValue value;
    value.int_value = 0;
    return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_NIL), false, NULL);
}

Expr *parser_call(Parser *parser, Expr *callee)
{
    Token paren = parser->previous;
    Expr **arguments = NULL;
    int arg_count = 0;
    int capacity = 0;

    if (!parser_check(parser, TOKEN_RIGHT_PAREN))
    {
        do
        {
            if (arg_count >= 255)
            {
                parser_error_at_current(parser, "Cannot have more than 255 arguments");
            }
            Expr *arg = parser_expression(parser);
            if (arg_count >= capacity)
            {
                capacity = capacity == 0 ? 8 : capacity * 2;
                Expr **new_arguments = arena_alloc(parser->arena, sizeof(Expr *) * capacity);
                if (new_arguments == NULL)
                {
                    exit(1);
                }
                if (arguments != NULL && arg_count > 0)
                {
                    memcpy(new_arguments, arguments, sizeof(Expr *) * arg_count);
                }
                arguments = new_arguments;
            }
            arguments[arg_count++] = arg;
        } while (parser_match(parser, TOKEN_COMMA));
    }

    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after arguments");
    return ast_create_call_expr(parser->arena, callee, arguments, arg_count, &paren);
}

Expr *parser_array_access(Parser *parser, Expr *array)
{
    Token bracket = parser->previous;
    Expr *start = NULL;
    Expr *end = NULL;
    Expr *step = NULL;

    // Check if this is a slice starting with '..' (e.g., [..] or [..end] or [..end:step])
    if (parser_match(parser, TOKEN_RANGE))
    {
        // This is a slice from the beginning: arr[..] or arr[..end] or arr[..end:step]
        if (!parser_check(parser, TOKEN_RIGHT_BRACKET) && !parser_check(parser, TOKEN_COLON))
        {
            // Use parser_term to avoid consuming range operator in nested expressions
            end = parser_term(parser);
        }
        // Check for step: arr[..end:step] or arr[..:step]
        if (parser_match(parser, TOKEN_COLON))
        {
            step = parser_term(parser);
        }
        parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after slice");
        return ast_create_array_slice_expr(parser->arena, array, NULL, end, step, &bracket);
    }

    // Parse the first expression (could be index or slice start)
    // Use parser_term to avoid consuming range operator - we handle it explicitly below
    Expr *first = parser_term(parser);

    // Check if this is a slice with start: arr[start..] or arr[start..end] or arr[start..end:step]
    if (parser_match(parser, TOKEN_RANGE))
    {
        start = first;
        // Check if there's an end expression
        if (!parser_check(parser, TOKEN_RIGHT_BRACKET) && !parser_check(parser, TOKEN_COLON))
        {
            end = parser_term(parser);
        }
        // Check for step: arr[start..end:step] or arr[start..:step]
        if (parser_match(parser, TOKEN_COLON))
        {
            step = parser_term(parser);
        }
        parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after slice");
        return ast_create_array_slice_expr(parser->arena, array, start, end, step, &bracket);
    }

    // Regular array access: arr[index]
    parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after index");
    return ast_create_array_access_expr(parser->arena, array, first, &bracket);
}
