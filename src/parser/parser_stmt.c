#include "parser/parser_stmt.h"
#include "parser/parser_stmt_decl.h"
#include "parser/parser_stmt_control.h"
#include "parser/parser_util.h"
#include "parser/parser_expr.h"
#include "ast/ast_expr.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations for static functions in modules */
static void parser_collect_comments(Parser *parser);
static void parser_attach_comments(Parser *parser, Stmt *stmt);
static bool parser_is_keyword_token(SnTokenType type);
static Stmt *parser_pragma_pack_statement(Parser *parser);
static Stmt *parser_pragma_alias_statement(Parser *parser);

/* Include split modules */
#include "parser_stmt_util.c"
#include "parser_stmt_parse.c"
#include "parser_stmt_decl_handler.c"
#include "parser_stmt_pragma_import.c"

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
