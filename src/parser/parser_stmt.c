#include "parser/parser_stmt.h"
#include "parser/parser_stmt_decl.h"
#include "parser/parser_stmt_control.h"
#include "parser/parser_util.h"
#include "parser/parser_expr.h"
#include "ast/ast_expr.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

/* Forward declaration for pragma pack parsing */
static Stmt *parser_pragma_pack_statement(Parser *parser);
static Stmt *parser_pragma_alias_statement(Parser *parser);

/* Parse optional "as val" or "as ref" memory qualifier */
MemoryQualifier parser_memory_qualifier(Parser *parser)
{
    if (parser_match(parser, TOKEN_AS))
    {
        if (parser_match(parser, TOKEN_VAL))
        {
            return MEM_AS_VAL;
        }
        else if (parser_match(parser, TOKEN_REF))
        {
            return MEM_AS_REF;
        }
        else
        {
            parser_error_at_current(parser, "Expected 'val' or 'ref' after 'as'");
        }
    }
    return MEM_DEFAULT;
}

/* Parse optional "shared" or "private" function modifier */
FunctionModifier parser_function_modifier(Parser *parser)
{
    if (parser_match(parser, TOKEN_SHARED))
    {
        return FUNC_SHARED;
    }
    else if (parser_match(parser, TOKEN_PRIVATE))
    {
        return FUNC_PRIVATE;
    }
    return FUNC_DEFAULT;
}

int is_at_function_boundary(Parser *parser)
{
    if (parser_check(parser, TOKEN_DEDENT))
    {
        return 1;
    }
    if (parser_check(parser, TOKEN_FN))
    {
        return 1;
    }
    if (parser_check(parser, TOKEN_EOF))
    {
        return 1;
    }
    return 0;
}

Stmt *parser_indented_block(Parser *parser)
{
    if (!parser_check(parser, TOKEN_INDENT))
    {
        parser_error(parser, "Expected indented block");
        return NULL;
    }
    parser_advance(parser);

    int current_indent = parser->lexer->indent_stack[parser->lexer->indent_size - 1];
    Stmt **statements = NULL;
    int count = 0;
    int capacity = 0;

    while (!parser_is_at_end(parser) &&
           parser->lexer->indent_stack[parser->lexer->indent_size - 1] >= current_indent)
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
        }

        if (parser_check(parser, TOKEN_DEDENT))
        {
            break;
        }

        if (parser_check(parser, TOKEN_EOF))
        {
            break;
        }

        Stmt *stmt = parser_declaration(parser);

        /* Synchronize on error to prevent infinite loops */
        if (parser->panic_mode)
        {
            synchronize(parser);
        }

        if (stmt == NULL)
        {
            continue;
        }

        if (count >= capacity)
        {
            capacity = capacity == 0 ? 8 : capacity * 2;
            Stmt **new_statements = arena_alloc(parser->arena, sizeof(Stmt *) * capacity);
            if (new_statements == NULL)
            {
                exit(1);
            }
            if (statements != NULL && count > 0)
            {
                memcpy(new_statements, statements, sizeof(Stmt *) * count);
            }
            statements = new_statements;
        }
        statements[count++] = stmt;
    }

    if (parser_check(parser, TOKEN_DEDENT))
    {
        parser_advance(parser);
    }
    else if (parser->lexer->indent_stack[parser->lexer->indent_size - 1] < current_indent)
    {
        parser_error(parser, "Expected dedent to end block");
    }

    return ast_create_block_stmt(parser->arena, statements, count, NULL);
}

Stmt *parser_statement(Parser *parser)
{
    while (parser_match(parser, TOKEN_NEWLINE))
    {
    }

    if (parser_is_at_end(parser))
    {
        parser_error(parser, "Unexpected end of file");
        return NULL;
    }

    if (parser_match(parser, TOKEN_VAR))
    {
        return parser_var_declaration(parser);
    }
    if (parser_match(parser, TOKEN_IF))
    {
        return parser_if_statement(parser);
    }
    if (parser_match(parser, TOKEN_WHILE))
    {
        return parser_while_statement(parser, false);
    }
    if (parser_match(parser, TOKEN_FOR))
    {
        return parser_for_statement(parser, false);
    }
    if (parser_match(parser, TOKEN_BREAK))
    {
        Token keyword = parser->previous;
        if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_match(parser, TOKEN_NEWLINE))
        {
            parser_consume(parser, TOKEN_NEWLINE, "Expected newline after 'break'");
        }
        return ast_create_break_stmt(parser->arena, &keyword);
    }
    if (parser_match(parser, TOKEN_CONTINUE))
    {
        Token keyword = parser->previous;
        if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_match(parser, TOKEN_NEWLINE))
        {
            parser_consume(parser, TOKEN_NEWLINE, "Expected newline after 'continue'");
        }
        return ast_create_continue_stmt(parser->arena, &keyword);
    }
    if (parser_match(parser, TOKEN_RETURN))
    {
        return parser_return_statement(parser);
    }
    if (parser_match(parser, TOKEN_LEFT_BRACE))
    {
        return parser_block_statement(parser);
    }

    // Parse shared => block, shared while, shared for, or private => block
    if (parser_check(parser, TOKEN_SHARED))
    {
        Token block_token = parser->current;
        parser_advance(parser);  // consume shared

        // Check if followed by while or for (shared loop)
        if (parser_match(parser, TOKEN_WHILE))
        {
            return parser_while_statement(parser, true);
        }
        if (parser_match(parser, TOKEN_FOR))
        {
            return parser_for_statement(parser, true);
        }

        // Otherwise it's a shared block
        parser_consume(parser, TOKEN_ARROW, "Expected '=>' after shared");
        skip_newlines(parser);

        Stmt *block = parser_indented_block(parser);
        if (block == NULL)
        {
            block = ast_create_block_stmt(parser->arena, NULL, 0, &block_token);
        }
        block->as.block.modifier = BLOCK_SHARED;
        return block;
    }

    // Parse private => block
    if (parser_check(parser, TOKEN_PRIVATE))
    {
        Token block_token = parser->current;
        parser_advance(parser);  // consume private

        parser_consume(parser, TOKEN_ARROW, "Expected '=>' after private");
        skip_newlines(parser);

        Stmt *block = parser_indented_block(parser);
        if (block == NULL)
        {
            block = ast_create_block_stmt(parser->arena, NULL, 0, &block_token);
        }
        block->as.block.modifier = BLOCK_PRIVATE;
        return block;
    }

    // Parse lock(expr) => block
    if (parser_match(parser, TOKEN_LOCK))
    {
        Token lock_token = parser->previous;

        parser_consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'lock'");
        Expr *lock_expr = parser_expression(parser);
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after lock expression");
        parser_consume(parser, TOKEN_ARROW, "Expected '=>' after lock(...)");
        skip_newlines(parser);

        Stmt *body = parser_indented_block(parser);
        if (body == NULL)
        {
            body = ast_create_block_stmt(parser->arena, NULL, 0, &lock_token);
        }

        return ast_create_lock_stmt(parser->arena, lock_expr, body, &lock_token);
    }

    return parser_expression_statement(parser);
}

Stmt *parser_declaration(Parser *parser)
{
    while (parser_match(parser, TOKEN_NEWLINE))
    {
    }

    if (parser_is_at_end(parser))
    {
        parser_error(parser, "Unexpected end of file");
        return NULL;
    }

    if (parser_match(parser, TOKEN_VAR))
    {
        return parser_var_declaration(parser);
    }
    if (parser_match(parser, TOKEN_FN))
    {
        return parser_function_declaration(parser);
    }
    if (parser_match(parser, TOKEN_NATIVE))
    {
        /* Check for 'native fn' or 'native struct' */
        if (parser_match(parser, TOKEN_FN))
        {
            return parser_native_function_declaration(parser);
        }
        else if (parser_match(parser, TOKEN_STRUCT))
        {
            return parser_struct_declaration(parser, true);
        }
        else
        {
            parser_error_at_current(parser, "Expected 'fn' or 'struct' after 'native'");
            return NULL;
        }
    }
    if (parser_match(parser, TOKEN_STRUCT))
    {
        return parser_struct_declaration(parser, false);
    }
    if (parser_match(parser, TOKEN_IMPORT))
    {
        return parser_import_statement(parser);
    }
    if (parser_match(parser, TOKEN_PRAGMA_INCLUDE))
    {
        return parser_pragma_statement(parser, PRAGMA_INCLUDE);
    }
    if (parser_match(parser, TOKEN_PRAGMA_LINK))
    {
        return parser_pragma_statement(parser, PRAGMA_LINK);
    }
    if (parser_match(parser, TOKEN_PRAGMA_SOURCE))
    {
        return parser_pragma_statement(parser, PRAGMA_SOURCE);
    }
    if (parser_match(parser, TOKEN_PRAGMA_PACK))
    {
        return parser_pragma_pack_statement(parser);
    }
    if (parser_match(parser, TOKEN_PRAGMA_ALIAS))
    {
        return parser_pragma_alias_statement(parser);
    }
    if (parser_match(parser, TOKEN_KEYWORD_TYPE))
    {
        return parser_type_declaration(parser);
    }

    return parser_statement(parser);
}

/* Declaration parsing functions are in parser_stmt_decl.c:
 * - parser_var_declaration
 * - parser_function_declaration
 * - parser_native_function_declaration
 * - parser_native_function_type
 * - parser_type_declaration
 *
 * Control flow parsing functions are in parser_stmt_control.c:
 * - parser_return_statement
 * - parser_if_statement
 * - parser_while_statement
 * - parser_for_statement
 */

Stmt *parser_block_statement(Parser *parser)
{
    Token brace = parser->previous;
    Stmt **statements = NULL;
    int count = 0;
    int capacity = 0;

    symbol_table_push_scope(parser->symbol_table);

    while (!parser_is_at_end(parser))
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
        }
        if (parser_is_at_end(parser) || parser_check(parser, TOKEN_DEDENT))
            break;

        Stmt *stmt = parser_declaration(parser);
        if (stmt == NULL)
            continue;

        if (count >= capacity)
        {
            capacity = capacity == 0 ? 8 : capacity * 2;
            Stmt **new_statements = arena_alloc(parser->arena, sizeof(Stmt *) * capacity);
            if (new_statements == NULL)
            {
                exit(1);
            }
            if (statements != NULL && count > 0)
            {
                memcpy(new_statements, statements, sizeof(Stmt *) * count);
            }
            statements = new_statements;
        }
        statements[count++] = stmt;
    }

    symbol_table_pop_scope(parser->symbol_table);

    return ast_create_block_stmt(parser->arena, statements, count, &brace);
}

Stmt *parser_expression_statement(Parser *parser)
{
    Expr *expr = parser_expression(parser);

    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after expression");
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
    }

    return ast_create_expr_stmt(parser->arena, expr, &parser->previous);
}

/* Helper to check if a token type is a reserved keyword */
static bool parser_is_keyword_token(SnTokenType type)
{
    switch (type)
    {
        case TOKEN_FN:
        case TOKEN_VAR:
        case TOKEN_RETURN:
        case TOKEN_IF:
        case TOKEN_ELSE:
        case TOKEN_FOR:
        case TOKEN_WHILE:
        case TOKEN_BREAK:
        case TOKEN_CONTINUE:
        case TOKEN_IN:
        case TOKEN_IMPORT:
        case TOKEN_NIL:
        case TOKEN_INT:
        case TOKEN_LONG:
        case TOKEN_DOUBLE:
        case TOKEN_CHAR:
        case TOKEN_STR:
        case TOKEN_BOOL:
        case TOKEN_BYTE:
        case TOKEN_VOID:
        case TOKEN_SHARED:
        case TOKEN_PRIVATE:
        case TOKEN_AS:
        case TOKEN_VAL:
        case TOKEN_REF:
        case TOKEN_BOOL_LITERAL:  /* true/false */
            return true;
        default:
            return false;
    }
}

Stmt *parser_pragma_statement(Parser *parser, PragmaType pragma_type)
{
    Token pragma_token = parser->previous;

    /* WYSIWYG pragma parsing: consume all tokens until newline/EOF
     * and preserve the exact text as written.
     *
     * Examples:
     *   #pragma include <math.h>        -> "<math.h>"
     *   #pragma include "runtime.h"     -> "\"runtime.h\""
     *   #pragma link pthread            -> "pthread"
     *
     * Note: Tokens are stored as duplicated strings in arena memory,
     * so we must concatenate them manually.
     */

    /* Check for old quoted syntax and error */
    if (parser_check(parser, TOKEN_STRING_LITERAL))
    {
        const char *str_val = parser->current.literal.string_value;
        if (str_val && str_val[0] == '<')
        {
            parser_error_at_current(parser,
                "Old pragma syntax detected. Use WYSIWYG syntax instead:\n"
                "  #pragma include <math.h>     (not \"<math.h>\")\n"
                "  #pragma include \"file.h\"   (not \"\\\"file.h\\\"\")");
            return NULL;
        }
    }

    /* Build pragma value by concatenating tokens */
    size_t capacity = 256;
    size_t len = 0;
    char *value = arena_alloc(parser->arena, capacity);
    value[0] = '\0';

    while (!parser_check(parser, TOKEN_NEWLINE) &&
           !parser_check(parser, TOKEN_SEMICOLON) &&
           !parser_is_at_end(parser))
    {
        Token tok = parser->current;

        /* For string literals, include the quotes */
        if (tok.type == TOKEN_STRING_LITERAL)
        {
            /* Need to add quotes around the string value */
            size_t str_len = strlen(tok.literal.string_value);
            size_t needed = len + str_len + 3; /* "..." + null */
            if (needed > capacity)
            {
                capacity = needed * 2;
                char *new_value = arena_alloc(parser->arena, capacity);
                memcpy(new_value, value, len);
                value = new_value;
            }
            value[len++] = '"';
            memcpy(value + len, tok.literal.string_value, str_len);
            len += str_len;
            value[len++] = '"';
            value[len] = '\0';
        }
        else
        {
            /* Append token text directly */
            size_t needed = len + tok.length + 1;
            if (needed > capacity)
            {
                capacity = needed * 2;
                char *new_value = arena_alloc(parser->arena, capacity);
                memcpy(new_value, value, len);
                value = new_value;
            }
            memcpy(value + len, tok.start, tok.length);
            len += tok.length;
            value[len] = '\0';
        }

        parser_advance(parser);
    }

    /* Handle case where no tokens were consumed */
    if (len == 0)
    {
        parser_error(parser, "Expected content after pragma directive");
        return NULL;
    }

    /* Consume optional semicolon */
    parser_match(parser, TOKEN_SEMICOLON);

    return ast_create_pragma_stmt(parser->arena, pragma_type, value, &pragma_token);
}

/* Parse #pragma pack(1) or #pragma pack()
 * #pragma pack(1) enables packed mode (no padding)
 * #pragma pack() resets to default alignment
 * Returns NULL since this doesn't create an AST node - it sets parser state */
static Stmt *parser_pragma_pack_statement(Parser *parser)
{
    Token pragma_token = parser->previous;

    parser_consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'pack'");

    if (parser_match(parser, TOKEN_RIGHT_PAREN))
    {
        /* #pragma pack() - reset to default alignment */
        parser->pack_alignment = 0;
    }
    else if (parser_match(parser, TOKEN_INT_LITERAL))
    {
        /* #pragma pack(N) - set alignment */
        int pack_value = (int)parser->previous.literal.int_value;
        if (pack_value != 1)
        {
            parser_error(parser, "Only #pragma pack(1) is supported");
            return NULL;
        }
        parser->pack_alignment = 1;
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after pack value");
    }
    else
    {
        parser_error_at_current(parser, "Expected integer literal or ')' in #pragma pack");
        return NULL;
    }

    // Consume optional newline/semicolon
    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after pragma directive");
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
    }

    /* Create a pragma statement to preserve the directive in the AST */
    const char *value = parser->pack_alignment == 1 ? "1" : "";
    return ast_create_pragma_stmt(parser->arena, PRAGMA_PACK, value, &pragma_token);
}

/* Parser for #pragma alias "c_name"
 * Sets pending_alias which will be applied to the next native struct, field, or method.
 * Returns a pragma statement to preserve the directive in the AST. */
static Stmt *parser_pragma_alias_statement(Parser *parser)
{
    Token pragma_token = parser->previous;

    /* Expect a string literal for the C alias name */
    if (!parser_match(parser, TOKEN_STRING_LITERAL))
    {
        parser_error_at_current(parser, "Expected string literal after #pragma alias");
        return NULL;
    }

    /* Extract the string value (strip quotes) */
    Token alias_token = parser->previous;
    const char *alias_start = alias_token.start + 1;  /* Skip opening quote */
    int alias_len = alias_token.length - 2;           /* Exclude both quotes */
    char *alias_value = arena_strndup(parser->arena, alias_start, alias_len);

    /* Store the pending alias */
    parser->pending_alias = alias_value;

    /* Consume optional newline/semicolon */
    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after pragma directive");
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
    }

    /* Create a pragma statement to preserve the directive in the AST */
    return ast_create_pragma_stmt(parser->arena, PRAGMA_ALIAS, alias_value, &pragma_token);
}

Stmt *parser_import_statement(Parser *parser)
{
    Token import_token = parser->previous;
    Token module_name;
    if (parser_match(parser, TOKEN_STRING_LITERAL))
    {
        module_name = parser->previous;
        module_name.start = arena_strdup(parser->arena, parser->previous.literal.string_value);
        if (module_name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
        module_name.length = strlen(module_name.start);
        module_name.line = parser->previous.line;
        module_name.filename = parser->previous.filename;
        module_name.type = TOKEN_STRING_LITERAL;
    }
    else
    {
        parser_error_at_current(parser, "Expected module name as string");
        module_name = parser->current;
        parser_advance(parser);
        module_name.start = arena_strndup(parser->arena, module_name.start, module_name.length);
        if (module_name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
    }

    /* Check for optional 'as namespace' clause */
    Token *namespace = NULL;
    if (parser_match(parser, TOKEN_AS))
    {
        /* Next token must be an identifier (not a keyword) */
        if (parser_check(parser, TOKEN_IDENTIFIER))
        {
            parser_advance(parser);
            namespace = arena_alloc(parser->arena, sizeof(Token));
            if (namespace == NULL)
            {
                parser_error_at_current(parser, "Out of memory");
                return NULL;
            }
            *namespace = parser->previous;
            namespace->start = arena_strndup(parser->arena, parser->previous.start, parser->previous.length);
            if (namespace->start == NULL)
            {
                parser_error_at_current(parser, "Out of memory");
                return NULL;
            }
        }
        else if (parser_is_keyword_token(parser->current.type))
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "Cannot use reserved keyword '%.*s' as namespace name",
                     parser->current.length, parser->current.start);
            parser_error_at_current(parser, msg);
            parser_advance(parser);  /* Skip the keyword to continue parsing */
        }
        else
        {
            parser_error_at_current(parser, "Expected namespace identifier after 'as'");
            if (!parser_check(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
            {
                parser_advance(parser);  /* Skip unexpected token */
            }
        }
    }

    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after import statement");
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
    }

    /* Create the import statement */
    Stmt *import_stmt = ast_create_import_stmt(parser->arena, module_name, namespace, &import_token);

    /* Import-first processing: immediately process the import to register types.
     * This allows struct types from the imported module to be used later in this file. */
    if (parser->import_ctx != NULL)
    {
        Module *imported_module = parser_process_import(parser, module_name.start, namespace != NULL);
        if (imported_module != NULL && import_stmt != NULL)
        {
            /* Store the imported statements for later processing (statement merging) */
            import_stmt->as.import.imported_stmts = imported_module->statements;
            import_stmt->as.import.imported_count = imported_module->count;
        }
    }

    return import_stmt;
}
