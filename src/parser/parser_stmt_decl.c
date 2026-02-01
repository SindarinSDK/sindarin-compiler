/**
 * parser_stmt_decl.c - Declaration parsing entry point
 *
 * Includes all declaration parsing modules.
 */

/* Include split modules directly (unity build style for smaller translation units) */
#include "parser_stmt_decl_var.c"
#include "parser_stmt_decl_func.c"
#include "parser_stmt_decl_native.c"
#include "parser_stmt_decl_struct.c"
#include "parser_stmt_decl_type.c"
