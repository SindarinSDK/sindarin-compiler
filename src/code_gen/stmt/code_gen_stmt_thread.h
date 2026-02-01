/*
 * code_gen_stmt_thread.h - Thread synchronization statement helpers
 */

#ifndef CODE_GEN_STMT_THREAD_H
#define CODE_GEN_STMT_THREAD_H

#include "code_gen.h"
#include "ast.h"

/* Generate thread sync statement */
void code_gen_thread_sync_statement(CodeGen *gen, Expr *expr, int indent);

#endif /* CODE_GEN_STMT_THREAD_H */
