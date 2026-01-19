#ifndef AST_STMT_H
#define AST_STMT_H

#include "ast.h"
#include "arena.h"

/* Statement creation functions */
Stmt *ast_create_expr_stmt(Arena *arena, Expr *expression, const Token *loc_token);
Stmt *ast_create_var_decl_stmt(Arena *arena, Token name, Type *type, Expr *initializer, const Token *loc_token);
Stmt *ast_create_function_stmt(Arena *arena, Token name, Parameter *params, int param_count,
                               Type *return_type, Stmt **body, int body_count, const Token *loc_token);
Stmt *ast_create_return_stmt(Arena *arena, Token keyword, Expr *value, const Token *loc_token);
Stmt *ast_create_block_stmt(Arena *arena, Stmt **statements, int count, const Token *loc_token);
Stmt *ast_create_if_stmt(Arena *arena, Expr *condition, Stmt *then_branch, Stmt *else_branch, const Token *loc_token);
Stmt *ast_create_while_stmt(Arena *arena, Expr *condition, Stmt *body, const Token *loc_token);
Stmt *ast_create_for_stmt(Arena *arena, Stmt *initializer, Expr *condition, Expr *increment, Stmt *body, const Token *loc_token);
Stmt *ast_create_break_stmt(Arena *arena, const Token *loc_token);
Stmt *ast_create_continue_stmt(Arena *arena, const Token *loc_token);
Stmt *ast_create_import_stmt(Arena *arena, Token module_name, Token *namespace, const Token *loc_token);
Stmt *ast_create_struct_decl_stmt(Arena *arena, Token name, StructField *fields, int field_count,
                                   StructMethod *methods, int method_count,
                                   bool is_native, bool is_packed, bool pass_self_by_ref,
                                   const char *c_alias, const Token *loc_token);

#endif /* AST_STMT_H */
