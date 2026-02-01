#include "parser_expr.h"
#include "parser_util.h"
#include "parser_stmt.h"
#include "parser_expr_interpol.h"
#include "parser_expr_match.h"
#include "parser_expr_struct.h"
#include "parser_expr_lambda.h"
#include "ast/ast_expr.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations are in parser_expr.h */

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
        compound_op = TOKEN_PLUS;
    else if (parser_match(parser, TOKEN_MINUS_EQUAL))
        compound_op = TOKEN_MINUS;
    else if (parser_match(parser, TOKEN_STAR_EQUAL))
        compound_op = TOKEN_STAR;
    else if (parser_match(parser, TOKEN_SLASH_EQUAL))
        compound_op = TOKEN_SLASH;
    else if (parser_match(parser, TOKEN_MODULO_EQUAL))
        compound_op = TOKEN_MODULO;
    else if (parser_match(parser, TOKEN_AMPERSAND_EQUAL))
        compound_op = TOKEN_AMPERSAND;
    else if (parser_match(parser, TOKEN_PIPE_EQUAL))
        compound_op = TOKEN_PIPE;
    else if (parser_match(parser, TOKEN_CARET_EQUAL))
        compound_op = TOKEN_CARET;
    else if (parser_match(parser, TOKEN_LSHIFT_EQUAL))
        compound_op = TOKEN_LSHIFT;
    else if (parser_match(parser, TOKEN_RSHIFT_EQUAL))
        compound_op = TOKEN_RSHIFT;

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
        Expr *right = parser_logical_and(parser);
        expr = ast_create_binary_expr(parser->arena, expr, op.type, right, &op);
    }
    return expr;
}

Expr *parser_logical_and(Parser *parser)
{
    Expr *expr = parser_bitwise_or(parser);
    while (parser_match(parser, TOKEN_AND))
    {
        Token op = parser->previous;
        Expr *right = parser_bitwise_or(parser);
        expr = ast_create_binary_expr(parser->arena, expr, op.type, right, &op);
    }
    return expr;
}

Expr *parser_bitwise_or(Parser *parser)
{
    Expr *expr = parser_bitwise_xor(parser);
    while (parser_match(parser, TOKEN_PIPE))
    {
        Token op = parser->previous;
        Expr *right = parser_bitwise_xor(parser);
        expr = ast_create_binary_expr(parser->arena, expr, op.type, right, &op);
    }
    return expr;
}

Expr *parser_bitwise_xor(Parser *parser)
{
    Expr *expr = parser_bitwise_and(parser);
    while (parser_match(parser, TOKEN_CARET))
    {
        Token op = parser->previous;
        Expr *right = parser_bitwise_and(parser);
        expr = ast_create_binary_expr(parser->arena, expr, op.type, right, &op);
    }
    return expr;
}

Expr *parser_bitwise_and(Parser *parser)
{
    Expr *expr = parser_equality(parser);
    while (parser_match(parser, TOKEN_AMPERSAND))
    {
        Token op = parser->previous;
        Expr *right = parser_equality(parser);
        expr = ast_create_binary_expr(parser->arena, expr, op.type, right, &op);
    }
    return expr;
}

Expr *parser_equality(Parser *parser)
{
    Expr *expr = parser_comparison(parser);
    while (parser_match(parser, TOKEN_BANG_EQUAL) || parser_match(parser, TOKEN_EQUAL_EQUAL))
    {
        Token op = parser->previous;
        Expr *right = parser_comparison(parser);
        expr = ast_create_binary_expr(parser->arena, expr, op.type, right, &op);
    }
    return expr;
}

Expr *parser_comparison(Parser *parser)
{
    Expr *expr = parser_shift(parser);
    while (parser_match(parser, TOKEN_LESS) || parser_match(parser, TOKEN_LESS_EQUAL) ||
           parser_match(parser, TOKEN_GREATER) || parser_match(parser, TOKEN_GREATER_EQUAL))
    {
        Token op = parser->previous;
        Expr *right = parser_shift(parser);
        expr = ast_create_binary_expr(parser->arena, expr, op.type, right, &op);
    }
    return expr;
}

Expr *parser_shift(Parser *parser)
{
    Expr *expr = parser_range(parser);
    while (parser_match(parser, TOKEN_LSHIFT) || parser_match(parser, TOKEN_RSHIFT))
    {
        Token op = parser->previous;
        Expr *right = parser_range(parser);
        expr = ast_create_binary_expr(parser->arena, expr, op.type, right, &op);
    }
    return expr;
}

Expr *parser_range(Parser *parser)
{
    Expr *expr = parser_term(parser);
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
        Expr *right = parser_factor(parser);
        expr = ast_create_binary_expr(parser->arena, expr, op.type, right, &op);
    }
    return expr;
}

Expr *parser_factor(Parser *parser)
{
    Expr *expr = parser_unary(parser);
    while (parser_match(parser, TOKEN_STAR) || parser_match(parser, TOKEN_SLASH) ||
           parser_match(parser, TOKEN_MODULO))
    {
        Token op = parser->previous;
        Expr *right = parser_unary(parser);
        expr = ast_create_binary_expr(parser->arena, expr, op.type, right, &op);
    }
    return expr;
}

Expr *parser_unary(Parser *parser)
{
    if (parser_match(parser, TOKEN_BANG) || parser_match(parser, TOKEN_MINUS) ||
        parser_match(parser, TOKEN_TILDE))
    {
        Token op = parser->previous;
        Expr *right = parser_unary(parser);
        return ast_create_unary_expr(parser->arena, op.type, right, &op);
    }

    /* typeof operator */
    if (parser_match(parser, TOKEN_TYPEOF))
    {
        Token typeof_token = parser->previous;
        bool has_parens = parser_match(parser, TOKEN_LEFT_PAREN);

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
                parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after typeof type");
            return ast_create_typeof_expr(parser->arena, NULL, type_literal, &typeof_token);
        }
        else
        {
            Expr *operand = parser_unary(parser);
            if (has_parens)
                parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after typeof expression");
            return ast_create_typeof_expr(parser->arena, operand, NULL, &typeof_token);
        }
    }

    /* sizeof operator */
    if (parser_match(parser, TOKEN_SIZEOF))
    {
        Token sizeof_token = parser->previous;
        bool has_parens = parser_match(parser, TOKEN_LEFT_PAREN);

        if (parser_check(parser, TOKEN_INT) || parser_check(parser, TOKEN_INT32) ||
            parser_check(parser, TOKEN_UINT) || parser_check(parser, TOKEN_UINT32) ||
            parser_check(parser, TOKEN_LONG) || parser_check(parser, TOKEN_DOUBLE) ||
            parser_check(parser, TOKEN_FLOAT) || parser_check(parser, TOKEN_CHAR) ||
            parser_check(parser, TOKEN_STR) || parser_check(parser, TOKEN_BOOL) ||
            parser_check(parser, TOKEN_BYTE) || parser_check(parser, TOKEN_VOID) ||
            parser_check(parser, TOKEN_ANY) || parser_check(parser, TOKEN_STAR))
        {
            Type *type_operand = parser_type(parser);
            if (has_parens)
                parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after sizeof type");
            return ast_create_sizeof_type_expr(parser->arena, type_operand, &sizeof_token);
        }
        else if (parser_check(parser, TOKEN_IDENTIFIER))
        {
            Token id = parser->current;
            Symbol *type_symbol = symbol_table_lookup_type(parser->symbol_table, id);
            if (type_symbol != NULL && type_symbol->type != NULL &&
                type_symbol->type->kind == TYPE_STRUCT)
            {
                Type *type_operand = parser_type(parser);
                if (has_parens)
                    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after sizeof type");
                return ast_create_sizeof_type_expr(parser->arena, type_operand, &sizeof_token);
            }
            else
            {
                Expr *operand = parser_unary(parser);
                if (has_parens)
                    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after sizeof expression");
                return ast_create_sizeof_expr_expr(parser->arena, operand, &sizeof_token);
            }
        }
        else
        {
            Expr *operand = parser_unary(parser);
            if (has_parens)
                parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after sizeof expression");
            return ast_create_sizeof_expr_expr(parser->arena, operand, &sizeof_token);
        }
    }

    /* Thread spawn: &fn() or &fn()! */
    if (parser_match(parser, TOKEN_AMPERSAND))
    {
        Token ampersand = parser->previous;
        Expr *call_expr = parser_primary(parser);
        if (call_expr == NULL)
        {
            parser_error(parser, "Expected function call after '&'");
            return NULL;
        }
        /* Handle only call-related postfix operations (not !) */
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
        if (call_expr->type != EXPR_CALL && call_expr->type != EXPR_STATIC_CALL)
        {
            parser_error(parser, "Thread spawn '&' requires a function call");
            return NULL;
        }
        Expr *spawn = ast_create_thread_spawn_expr(parser->arena, call_expr, FUNC_DEFAULT, &ampersand);
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
            Token bang = parser->previous;
            bool is_sync_list = (expr->type == EXPR_SYNC_LIST);
            expr = ast_create_thread_sync_expr(parser->arena, expr, is_sync_list, &bang);
        }
        else if (parser_match(parser, TOKEN_AS))
        {
            Token as_token = parser->previous;
            if (parser_match(parser, TOKEN_VAL))
            {
                expr = ast_create_as_val_expr(parser->arena, expr, &as_token);
            }
            else if (parser_match(parser, TOKEN_REF))
            {
                expr = ast_create_as_ref_expr(parser->arena, expr, &as_token);
            }
            else
            {
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
        else if (skip_whitespace_for_continuation(parser))
        {
            continue;
        }
        else
        {
            break;
        }
    }

    consume_continuation_dedents(parser);
    return expr;
}

Expr *parser_primary(Parser *parser)
{
    /* Literal tokens */
    if (parser_match(parser, TOKEN_INT_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return ast_create_literal_expr(parser->arena, value,
            ast_create_primitive_type(parser->arena, TYPE_INT), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_LONG_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return ast_create_literal_expr(parser->arena, value,
            ast_create_primitive_type(parser->arena, TYPE_LONG), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_BYTE_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return ast_create_literal_expr(parser->arena, value,
            ast_create_primitive_type(parser->arena, TYPE_BYTE), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_DOUBLE_LITERAL))
    {
        LiteralValue value;
        value.double_value = parser->previous.literal.double_value;
        return ast_create_literal_expr(parser->arena, value,
            ast_create_primitive_type(parser->arena, TYPE_DOUBLE), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_FLOAT_LITERAL))
    {
        LiteralValue value;
        value.double_value = parser->previous.literal.double_value;
        return ast_create_literal_expr(parser->arena, value,
            ast_create_primitive_type(parser->arena, TYPE_FLOAT), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_UINT_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return ast_create_literal_expr(parser->arena, value,
            ast_create_primitive_type(parser->arena, TYPE_UINT), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_UINT32_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return ast_create_literal_expr(parser->arena, value,
            ast_create_primitive_type(parser->arena, TYPE_UINT32), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_INT32_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return ast_create_literal_expr(parser->arena, value,
            ast_create_primitive_type(parser->arena, TYPE_INT32), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_CHAR_LITERAL))
    {
        LiteralValue value;
        value.char_value = parser->previous.literal.char_value;
        return ast_create_literal_expr(parser->arena, value,
            ast_create_primitive_type(parser->arena, TYPE_CHAR), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_STRING_LITERAL))
    {
        if (parser->previous.literal.string_value == NULL)
        {
            parser_error(parser, "Invalid string literal");
            return NULL;
        }
        LiteralValue value;
        value.string_value = arena_strdup(parser->arena, parser->previous.literal.string_value);
        return ast_create_literal_expr(parser->arena, value,
            ast_create_primitive_type(parser->arena, TYPE_STRING), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_BOOL_LITERAL))
    {
        LiteralValue value;
        value.bool_value = parser->previous.literal.bool_value;
        return ast_create_literal_expr(parser->arena, value,
            ast_create_primitive_type(parser->arena, TYPE_BOOL), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_NIL))
    {
        LiteralValue value;
        value.int_value = 0;
        return ast_create_literal_expr(parser->arena, value,
            ast_create_primitive_type(parser->arena, TYPE_NIL), false, &parser->previous);
    }

    /* Lambda expression */
    if (parser_match(parser, TOKEN_FN))
    {
        Token fn_token = parser->previous;
        return parse_lambda_expr(parser, &fn_token);
    }

    /* Identifier - could be variable, struct literal, or static call */
    if (parser_match(parser, TOKEN_IDENTIFIER))
    {
        Token var_token = parser->previous;

        /* Check for struct literal */
        if (parser_check(parser, TOKEN_LEFT_BRACE))
        {
            Symbol *type_symbol = symbol_table_lookup_type(parser->symbol_table, var_token);
            if (type_symbol != NULL && type_symbol->type != NULL &&
                type_symbol->type->kind == TYPE_STRUCT)
            {
                return parse_struct_literal(parser, &var_token);
            }
        }

        /* Check for static method call */
        bool is_static_type = parser_is_static_type_name(var_token.start, var_token.length);
        if (!is_static_type && parser_check(parser, TOKEN_DOT))
        {
            Symbol *type_symbol = symbol_table_lookup_type(parser->symbol_table, var_token);
            if (type_symbol != NULL && type_symbol->type != NULL &&
                type_symbol->type->kind == TYPE_STRUCT)
            {
                is_static_type = true;
            }
        }
        if (parser_check(parser, TOKEN_DOT) && is_static_type)
        {
            return parse_static_call(parser, &var_token);
        }

        /* Regular variable */
        var_token.start = arena_strndup(parser->arena, parser->previous.start, parser->previous.length);
        return ast_create_variable_expr(parser->arena, var_token, &parser->previous);
    }

    /* Grouped expression */
    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        Expr *expr = parser_expression(parser);
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }

    /* Array literal */
    if (parser_match(parser, TOKEN_LEFT_BRACE))
    {
        Token left_brace = parser->previous;
        Expr **elements = NULL;
        int count = 0;
        int capacity = 0;

        while (parser_match(parser, TOKEN_NEWLINE) ||
               parser_match(parser, TOKEN_INDENT) ||
               parser_match(parser, TOKEN_DEDENT)) {}

        if (!parser_check(parser, TOKEN_RIGHT_BRACE))
        {
            do
            {
                while (parser_match(parser, TOKEN_NEWLINE) ||
                       parser_match(parser, TOKEN_INDENT) ||
                       parser_match(parser, TOKEN_DEDENT)) {}

                Expr *elem;
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

        while (parser_match(parser, TOKEN_NEWLINE) ||
               parser_match(parser, TOKEN_INDENT) ||
               parser_match(parser, TOKEN_DEDENT)) {}

        parser_consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after array elements");
        return ast_create_array_expr(parser->arena, elements, count, &left_brace);
    }

    /* Sync list */
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

    /* Interpolated string */
    if (parser_match(parser, TOKEN_INTERPOL_STRING))
    {
        Token interpol_token = parser->previous;
        return parse_interpol_string(parser, &interpol_token);
    }

    /* Match expression */
    if (parser_match(parser, TOKEN_MATCH))
    {
        Token match_token = parser->previous;
        return parse_match_expr(parser, &match_token);
    }

    parser_error_at_current(parser, "Expected expression");
    LiteralValue value;
    value.int_value = 0;
    return ast_create_literal_expr(parser->arena, value,
        ast_create_primitive_type(parser->arena, TYPE_NIL), false, NULL);
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

    /* Check for slice starting with '..' */
    if (parser_match(parser, TOKEN_RANGE))
    {
        if (!parser_check(parser, TOKEN_RIGHT_BRACKET) && !parser_check(parser, TOKEN_COLON))
        {
            end = parser_term(parser);
        }
        if (parser_match(parser, TOKEN_COLON))
        {
            step = parser_term(parser);
        }
        parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after slice");
        return ast_create_array_slice_expr(parser->arena, array, NULL, end, step, &bracket);
    }

    Expr *first = parser_term(parser);

    /* Check for slice with start */
    if (parser_match(parser, TOKEN_RANGE))
    {
        start = first;
        if (!parser_check(parser, TOKEN_RIGHT_BRACKET) && !parser_check(parser, TOKEN_COLON))
        {
            end = parser_term(parser);
        }
        if (parser_match(parser, TOKEN_COLON))
        {
            step = parser_term(parser);
        }
        parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after slice");
        return ast_create_array_slice_expr(parser->arena, array, start, end, step, &bracket);
    }

    parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after index");
    return ast_create_array_access_expr(parser->arena, array, first, &bracket);
}
