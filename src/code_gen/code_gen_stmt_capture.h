#ifndef CODE_GEN_STMT_CAPTURE_H
#define CODE_GEN_STMT_CAPTURE_H

#include "code_gen.h"
#include "ast.h"

/* Pre-pass to identify primitives captured by closures in a function body.
 * These need to be declared as heap-allocated pointers so mutations persist. */
void code_gen_scan_captured_primitives(CodeGen *gen, Stmt **stmts, int stmt_count);

/* Check if a variable name is a captured primitive */
bool code_gen_is_captured_primitive(CodeGen *gen, const char *name);

/* Clear the captured primitives list (call at end of function) */
void code_gen_clear_captured_primitives(CodeGen *gen);

/* Private block arena stack operations */
void push_arena_to_stack(CodeGen *gen, const char *arena_name);
const char *pop_arena_from_stack(CodeGen *gen);

#endif /* CODE_GEN_STMT_CAPTURE_H */
