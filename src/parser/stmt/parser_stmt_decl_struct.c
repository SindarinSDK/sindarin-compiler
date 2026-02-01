/**
 * parser_stmt_decl_struct.c - Struct declaration parsing
 *
 * Contains functions for parsing struct declarations including fields and methods.
 */

#include "parser/stmt/parser_stmt_decl.h"
#include "parser/util/parser_util.h"
#include "parser/expr/parser_expr.h"
#include "parser/stmt/parser_stmt.h"
#include "ast/ast_expr.h"
#include "ast/ast_stmt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helper function to parse a struct method declaration */
static StructMethod *parser_struct_method(Parser *parser, bool is_static, bool is_native_method, FunctionModifier modifier)
{
    Token method_name;

    if (parser_check_method_name(parser))
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
    bool has_arena_param = false;

    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        /* Check for 'arena' identifier as first parameter (implicit arena passing) - only for native methods */
        if (is_native_method &&
            parser_check(parser, TOKEN_IDENTIFIER) &&
            parser->current.length == 5 &&
            memcmp(parser->current.start, "arena", 5) == 0)
        {
            /* Peek ahead to see if this is the contextual 'arena' keyword (followed by , or )) */
            Token next = parser_peek_token(parser);
            if (next.type == TOKEN_COMMA || next.type == TOKEN_RIGHT_PAREN)
            {
                parser_advance(parser);  /* consume 'arena' */
                has_arena_param = true;
                /* Consume comma if there are more parameters */
                if (!parser_check(parser, TOKEN_RIGHT_PAREN))
                {
                    parser_consume(parser, TOKEN_COMMA, "Expected ',' after 'arena'");
                }
            }
        }

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
                params[param_count].sync_modifier = SYNC_NONE;
                param_count++;
            } while (parser_match(parser, TOKEN_COMMA));
        }
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");
    }
    (void)is_variadic;  /* Struct methods don't use variadic flag yet */

    /* Check for misplaced modifiers after parameters */
    if (parser_check(parser, TOKEN_SHARED) || parser_check(parser, TOKEN_PRIVATE) || parser_check(parser, TOKEN_STATIC))
    {
        const char *keyword = parser_check(parser, TOKEN_SHARED) ? "shared" :
                              parser_check(parser, TOKEN_PRIVATE) ? "private" : "static";
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
            "'%s' must be declared before 'fn', not after the parameter list. "
            "Example: %s fn %.*s(...): type => ...",
            keyword, keyword, method_name.length, method_name.start);
        parser_error_at_current(parser, error_msg);
        parser_advance(parser);
    }

    FunctionModifier func_modifier = modifier;

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
    method->has_arena_param = has_arena_param;
    method->name_token = method_name;
    method->c_alias = NULL;  /* Set via #pragma alias if needed */

    return method;
}

/* Helper function to check if the current tokens indicate a method declaration */
static bool parser_is_method_start(Parser *parser)
{
    /* Methods can start with any combination of: shared/private, static, native, fn */
    if (parser_check(parser, TOKEN_FN))
    {
        return true;
    }
    if (parser_check(parser, TOKEN_STATIC))
    {
        return true;
    }
    if (parser_check(parser, TOKEN_SHARED) || parser_check(parser, TOKEN_PRIVATE))
    {
        return true;
    }
    if (parser_check(parser, TOKEN_NATIVE))
    {
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
                /* Parse method modifiers (shared/private/static/native in any order before fn) */
                bool is_method_static = false;
                bool is_method_native = false;
                FunctionModifier method_modifier = FUNC_DEFAULT;
                bool modifier_done = false;

                while (!modifier_done && !parser_is_at_end(parser))
                {
                    if (parser_check(parser, TOKEN_STATIC))
                    {
                        if (is_method_static)
                        {
                            parser_error_at_current(parser, "Duplicate 'static' modifier");
                        }
                        is_method_static = true;
                        parser_advance(parser);
                    }
                    else if (parser_check(parser, TOKEN_SHARED))
                    {
                        if (method_modifier == FUNC_PRIVATE)
                        {
                            parser_error_at_current(parser,
                                "'shared' and 'private' cannot be used together. "
                                "A function is either shared (uses caller's arena) or private (isolated arena)");
                        }
                        if (method_modifier == FUNC_SHARED)
                        {
                            parser_error_at_current(parser, "Duplicate 'shared' modifier");
                        }
                        method_modifier = FUNC_SHARED;
                        parser_advance(parser);
                    }
                    else if (parser_check(parser, TOKEN_PRIVATE))
                    {
                        if (method_modifier == FUNC_SHARED)
                        {
                            parser_error_at_current(parser,
                                "'shared' and 'private' cannot be used together. "
                                "A function is either shared (uses caller's arena) or private (isolated arena)");
                        }
                        if (method_modifier == FUNC_PRIVATE)
                        {
                            parser_error_at_current(parser, "Duplicate 'private' modifier");
                        }
                        method_modifier = FUNC_PRIVATE;
                        parser_advance(parser);
                    }
                    else if (parser_check(parser, TOKEN_NATIVE))
                    {
                        if (is_method_native)
                        {
                            parser_error_at_current(parser, "Duplicate 'native' modifier");
                        }
                        is_method_native = true;
                        parser_advance(parser);
                    }
                    else
                    {
                        modifier_done = true;
                    }
                }

                /* Now we should be at 'fn' */
                if (!parser_match(parser, TOKEN_FN))
                {
                    parser_error_at_current(parser, "Expected 'fn' after method modifiers");
                    continue;
                }

                /* Parse method */
                StructMethod *method = parser_struct_method(parser, is_method_static, is_method_native, method_modifier);
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
