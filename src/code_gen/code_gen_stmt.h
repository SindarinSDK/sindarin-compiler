#ifndef CODE_GEN_STMT_H
#define CODE_GEN_STMT_H

#include "code_gen.h"
#include "ast.h"
#include "code_gen/code_gen_stmt_loop.h"
#include "code_gen/code_gen_stmt_capture.h"

/* Statement code generation functions */
void code_gen_expression_statement(CodeGen *gen, ExprStmt *stmt, int indent);
void code_gen_var_declaration(CodeGen *gen, VarDeclStmt *stmt, int indent);
void code_gen_free_locals(CodeGen *gen, Scope *scope, bool is_function, int indent);
void code_gen_block(CodeGen *gen, BlockStmt *stmt, int indent);
void code_gen_function(CodeGen *gen, FunctionStmt *stmt);
void code_gen_return_statement(CodeGen *gen, ReturnStmt *stmt, int indent);
void code_gen_if_statement(CodeGen *gen, IfStmt *stmt, int indent);
void code_gen_statement(CodeGen *gen, Stmt *stmt, int indent);

#endif /* CODE_GEN_STMT_H */
