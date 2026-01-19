#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "arena.h"
#include "ast.h"
#include "symbol_table.h"

/* Forward declaration for import context */
typedef struct ImportContext ImportContext;

typedef struct
{
    Lexer *lexer;
    Token current;
    Token previous;
    int had_error;
    int panic_mode;
    SymbolTable *symbol_table;
    char **interp_sources;
    int interp_count;
    int interp_capacity;
    Arena *arena;
    int sized_array_pending;   /* Set when parser_type() detects TYPE[expr] syntax */
    Expr *sized_array_size;    /* Size expression parsed from TYPE[expr] syntax */
    int in_native_function;    /* True when parsing body of native function, for native lambdas */
    int pack_alignment;        /* Current pack alignment: 0 = default, 1 = packed */
    ImportContext *import_ctx; /* Context for import-first processing (NULL if not tracking imports) */
    const char *pending_alias; /* C alias from #pragma alias, applied to next declaration */
} Parser;

/* Forward declare Parser for function pointer type */
struct Parser;

/* Import context for tracking imported modules and enabling import-first processing */
struct ImportContext
{
    char **imported;              /* Array of imported file paths */
    int *imported_count;          /* Pointer to count (shared across recursive calls) */
    int *imported_capacity;       /* Pointer to capacity (shared across recursive calls) */
    Module **imported_modules;    /* Array of parsed imported modules */
    bool *imported_directly;      /* Array tracking if each import is direct (non-namespaced) */
    const char *current_file;     /* Path of the file currently being parsed */
    const char *compiler_dir;     /* Directory containing compiler (for SDK resolution) */
    /* Function pointer for recursive import processing (to avoid circular dependency) */
    Module *(*process_import)(Arena *arena, SymbolTable *symbol_table, const char *import_path,
                              struct ImportContext *ctx);
};

/* Result struct for parser_type_with_size() */
typedef struct
{
    Type *type;           /* The parsed type (element type if sized array) */
    Expr *size_expr;      /* Size expression if TYPE[expr] syntax, NULL otherwise */
    int is_sized_array;   /* 1 if sized array syntax was detected */
} ParsedType;

void parser_init(Arena *arena, Parser *parser, Lexer *lexer, SymbolTable *symbol_table);
void parser_cleanup(Parser *parser);

void parser_error(Parser *parser, const char *message);
void parser_error_at_current(Parser *parser, const char *message);
void parser_error_at(Parser *parser, Token *token, const char *message);

void parser_advance(Parser *parser);
void parser_consume(Parser *parser, SnTokenType type, const char *message);
int parser_check(Parser *parser, SnTokenType type);
int parser_match(Parser *parser, SnTokenType type);

Type *parser_type(Parser *parser);
ParsedType parser_type_with_size(Parser *parser);
MemoryQualifier parser_memory_qualifier(Parser *parser);
FunctionModifier parser_function_modifier(Parser *parser);

Expr *parser_expression(Parser *parser);
Expr *parser_assignment(Parser *parser);
Expr *parser_logical_or(Parser *parser);
Expr *parser_logical_and(Parser *parser);
Expr *parser_equality(Parser *parser);
Expr *parser_comparison(Parser *parser);
Expr *parser_term(Parser *parser);
Expr *parser_factor(Parser *parser);
Expr *parser_unary(Parser *parser);
Expr *parser_postfix(Parser *parser);
Expr *parser_primary(Parser *parser);
Expr *parser_call(Parser *parser, Expr *callee);
Expr *parser_array_access(Parser *parser, Expr *array);

Stmt *parser_statement(Parser *parser);
Stmt *parser_declaration(Parser *parser);
Stmt *parser_var_declaration(Parser *parser);
Stmt *parser_function_declaration(Parser *parser);
Stmt *parser_return_statement(Parser *parser);
Stmt *parser_if_statement(Parser *parser);
Stmt *parser_while_statement(Parser *parser, bool is_shared);
Stmt *parser_for_statement(Parser *parser, bool is_shared);
Stmt *parser_block_statement(Parser *parser);
Stmt *parser_expression_statement(Parser *parser);
Stmt *parser_import_statement(Parser *parser);

Module *parser_execute(Parser *parser, const char *filename);
Module *parse_module_with_imports(Arena *arena, SymbolTable *symbol_table, const char *filename,
                                  char ***imported, int *imported_count, int *imported_capacity,
                                  Module ***imported_modules, bool **imported_directly,
                                  const char *compiler_dir);

/* Process an import immediately during parsing - called by parser_import_statement.
 * Returns the parsed module, or NULL if import context is not available or on error.
 * When successful, types from the imported module are registered in the symbol table. */
Module *parser_process_import(Parser *parser, const char *module_name, bool is_namespaced);

#endif