#ifndef CODE_GEN_STMT_LOOP_H
#define CODE_GEN_STMT_LOOP_H

#include "code_gen.h"
#include "ast.h"

/* Loop arena management - for per-iteration arenas in loops */
void push_loop_arena(CodeGen *gen, char *arena_var, char *cleanup_label);
void pop_loop_arena(CodeGen *gen);

/* Loop statement code generation */
void code_gen_while_statement(CodeGen *gen, WhileStmt *stmt, int indent);
void code_gen_for_statement(CodeGen *gen, ForStmt *stmt, int indent);
void code_gen_for_each_statement(CodeGen *gen, ForEachStmt *stmt, int indent);

/* Loop counter tracking for optimization - tracks variables known to be non-negative */
void push_loop_counter(CodeGen *gen, const char *var_name);
void pop_loop_counter(CodeGen *gen);
bool is_tracked_loop_counter(CodeGen *gen, const char *var_name);

#endif /* CODE_GEN_STMT_LOOP_H */
