#ifndef CODE_GEN_EXPR_LAMBDA_H
#define CODE_GEN_EXPR_LAMBDA_H

#include "ast.h"
#include "symbol_table.h"
#include "arena.h"
#include "code_gen.h"
#include <stdbool.h>

/* ============================================================================
 * Lambda Expression Code Generation Helpers
 * ============================================================================
 * Structures and functions for tracking variables in lambda expressions.
 * ============================================================================ */

/* Helper structure for local variable tracking in lambda bodies */
typedef struct {
    char **names;
    int count;
    int capacity;
} LocalVars;

/* Helper structure for tracking enclosing lambda parameters */
typedef struct EnclosingLambdaContext {
    LambdaExpr **lambdas;
    int count;
    int capacity;
} EnclosingLambdaContext;

/* Helper structure for captured variable tracking in lambda bodies */
typedef struct {
    char **names;
    Type **types;
    int count;
    int capacity;
} CapturedVars;

/* Check if a name is a parameter of any enclosing lambda, and get its type */
Type *find_enclosing_lambda_param(EnclosingLambdaContext *ctx, const char *name);

/* Check if a name is a parameter of a specific lambda */
bool is_lambda_param(LambdaExpr *lambda, const char *name);

/* Check if a name is a local variable in the current lambda scope */
bool is_local_var(LocalVars *lv, const char *name);

/* Initialize captured vars structure */
void captured_vars_init(CapturedVars *cv);

/* Add a captured variable (de-duplicated) */
void captured_vars_add(CapturedVars *cv, Arena *arena, const char *name, Type *type);

/* Forward declaration for statement traversal */
void collect_captured_vars_from_stmt(Stmt *stmt, LambdaExpr *lambda, SymbolTable *table,
                                     CapturedVars *cv, LocalVars *lv, EnclosingLambdaContext *enclosing, Arena *arena);

/* Recursively collect captured variables from an expression */
void collect_captured_vars(Expr *expr, LambdaExpr *lambda, SymbolTable *table,
                           CapturedVars *cv, LocalVars *lv, EnclosingLambdaContext *enclosing, Arena *arena);

/* Generate statement body code for lambda
 * Sets up context so return statements work correctly */
char *code_gen_lambda_stmt_body(CodeGen *gen, LambdaExpr *lambda, int indent,
                                 const char *lambda_func_name, Type *return_type);

/* is_primitive_type is defined in type_checker_util.h */

/* Initialize LocalVars structure */
void local_vars_init(LocalVars *lv);

/* Add a local variable name (de-duplicated) */
void local_vars_add(LocalVars *lv, Arena *arena, const char *name);

/* Collect local variable declarations from a statement */
void collect_local_vars_from_stmt(Stmt *stmt, LocalVars *lv, Arena *arena);

/* Generate code for a lambda expression */
char *code_gen_lambda_expression(CodeGen *gen, Expr *expr);

#endif /* CODE_GEN_EXPR_LAMBDA_H */
