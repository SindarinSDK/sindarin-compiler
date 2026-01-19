#ifndef CODE_GEN_H
#define CODE_GEN_H

#include "arena.h"
#include "ast.h"
#include "symbol_table.h"
#include <stdio.h>
#include <stdbool.h>

/* Arithmetic mode for code generation */
typedef enum {
    ARITH_CHECKED,     /* Use runtime functions with overflow checking (default) */
    ARITH_UNCHECKED    /* Use native C operators without overflow checking */
} ArithmeticMode;

/* Pragma source with location info for validation */
typedef struct PragmaSourceInfo {
    const char *value;      /* The pragma value (e.g., "helper.c") */
    const char *source_dir; /* Directory of the .sn file where pragma is defined */
} PragmaSourceInfo;

typedef struct {
    Arena *arena;
    int label_count;
    SymbolTable *symbol_table;
    FILE *output;
    char *current_function;
    Type *current_return_type;
    int temp_count;
    char *for_continue_label;  // Label to jump to for continue in for loops

    /* Arena context for memory management */
    int arena_depth;            // Current arena nesting level
    bool in_shared_context;     // Are we in a shared block/loop?
    bool in_private_context;    // Are we in a private block/function?
    char *current_arena_var;    // Name of current arena variable (e.g., "__arena__")
    FunctionModifier current_func_modifier;  // Current function's modifier

    /* Loop arena for per-iteration cleanup */
    char *loop_arena_var;       // Name of current loop's per-iteration arena (NULL if shared loop)
    char *loop_cleanup_label;   // Label for loop cleanup (used by break/continue)

    /* Loop arena stack for nested loops - tracks active loop arenas for proper cleanup */
    char **loop_arena_stack;    // Stack of loop arena variable names
    char **loop_cleanup_stack;  // Stack of loop cleanup label names
    int loop_arena_depth;       // Current loop nesting depth (0 = not in loop)
    int loop_arena_capacity;    // Capacity of loop arena stacks

    /* Loop counter tracking for optimization - tracks variables known to be non-negative */
    char **loop_counter_names;  // Names of loop counter variables (provably non-negative)
    int loop_counter_count;     // Number of tracked loop counters
    int loop_counter_capacity;  // Capacity of loop counter array

    /* Private block arena stack - tracks arena names for nested private blocks */
    char **arena_stack;         // Stack of arena variable names for private blocks
    int arena_stack_depth;      // Current depth of private block nesting
    int arena_stack_capacity;   // Capacity of arena stack

    /* Lambda support */
    int lambda_count;           // Counter for unique lambda IDs
    char *lambda_forward_decls; // Buffer for lambda forward declarations
    char *lambda_definitions;   // Buffer for lambda function bodies

    /* Thread wrapper support */
    int thread_wrapper_count;   // Counter for unique thread wrapper IDs

    /* Named function to closure wrapper support */
    int wrapper_count;          // Counter for unique wrapper function IDs

    /* Enclosing lambda tracking for nested lambda capture */
    LambdaExpr **enclosing_lambdas;
    int enclosing_lambda_count;
    int enclosing_lambda_capacity;

    /* Buffered output for correct ordering */
    char *function_definitions; // Buffer for user function definitions
    bool buffering_functions;   // Are we buffering to function_definitions?

    /* Optimization settings */
    ArithmeticMode arithmetic_mode;  // Checked or unchecked arithmetic

    /* Tail call optimization state */
    bool in_tail_call_function;     // Are we generating a tail-call-optimized function?
    FunctionStmt *tail_call_fn;     // The function being optimized (for param access)

    /* Captured variable tracking for closure mutation persistence.
     * Primitive variables that are captured by closures need special handling:
     * they must be heap-allocated so mutations persist across closure calls
     * and are visible to the outer scope. */
    char **captured_primitives;     // Names of captured primitive variables
    long **captured_prim_ptrs;      // Pointers to arena-allocated storage (for redirection)
    int captured_prim_count;
    int captured_prim_capacity;

    /* Pragma tracking for C interop */
    char **pragma_includes;         // List of include directives (e.g., "<math.h>")
    int pragma_include_count;
    int pragma_include_capacity;
    char **pragma_links;            // List of libraries to link (e.g., "m", "pthread")
    int pragma_link_count;
    int pragma_link_capacity;
    struct PragmaSourceInfo *pragma_sources;  // List of C source files with location info
    int pragma_source_count;
    int pragma_source_capacity;

    /* Interceptor thunk support */
    int thunk_count;                // Counter for unique thunk IDs
    char *thunk_forward_decls;      // Buffer for thunk forward declarations
    char *thunk_definitions;        // Buffer for thunk function bodies

    /* Array compound literal context - when true, struct literals should omit
     * outer type cast since the array type already establishes element type.
     * This is needed for TCC compatibility which doesn't handle nested casts. */
    bool in_array_compound_literal;

    /* Recursive lambda support - tracks the variable being declared when
     * initializing with a lambda. This allows lambdas to skip capturing
     * themselves during initialization, then we add a self-fix statement. */
    char *current_decl_var_name;         /* Name of variable being declared */
    int recursive_lambda_id;             /* Lambda ID if recursive, -1 otherwise */

    /* Return closure allocation - when true, closures should be allocated
     * in __caller_arena__ instead of __local_arena__. This is set when
     * generating lambda expressions that are directly returned from functions. */
    bool allocate_closure_in_caller_arena;
} CodeGen;

void code_gen_init(Arena *arena, CodeGen *gen, SymbolTable *symbol_table, const char *output_file);
void code_gen_cleanup(CodeGen *gen);
int code_gen_new_label(CodeGen *gen);
void code_gen_module(CodeGen *gen, Module *module);
void code_gen_statement(CodeGen *gen, Stmt *stmt, int indent);
void code_gen_block(CodeGen *gen, BlockStmt *stmt, int indent);
void code_gen_function(CodeGen *gen, FunctionStmt *stmt);
void code_gen_return_statement(CodeGen *gen, ReturnStmt *stmt, int indent);
void code_gen_if_statement(CodeGen *gen, IfStmt *stmt, int indent);
void code_gen_while_statement(CodeGen *gen, WhileStmt *stmt, int indent);
void code_gen_for_statement(CodeGen *gen, ForStmt *stmt, int indent);

/* Include sub-module headers */
#include "code_gen/code_gen_util.h"
#include "code_gen/code_gen_stmt.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_expr_array.h"
#include "code_gen/code_gen_expr_call.h"
#include "code_gen/code_gen_expr_lambda.h"
#include "code_gen/code_gen_expr_static.h"
#include "code_gen/code_gen_expr_string.h"

#endif