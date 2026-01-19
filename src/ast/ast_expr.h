#ifndef AST_EXPR_H
#define AST_EXPR_H

#include "ast.h"
#include "arena.h"

/* Expression creation functions */
Expr *ast_create_binary_expr(Arena *arena, Expr *left, SnTokenType operator, Expr *right, const Token *loc_token);
Expr *ast_create_unary_expr(Arena *arena, SnTokenType operator, Expr *operand, const Token *loc_token);
Expr *ast_create_literal_expr(Arena *arena, LiteralValue value, Type *type, bool is_interpolated, const Token *loc_token);
Expr *ast_create_variable_expr(Arena *arena, Token name, const Token *loc_token);
Expr *ast_create_assign_expr(Arena *arena, Token name, Expr *value, const Token *loc_token);
Expr *ast_create_call_expr(Arena *arena, Expr *callee, Expr **arguments, int arg_count, const Token *loc_token);
Expr *ast_create_array_expr(Arena *arena, Expr **elements, int element_count, const Token *loc_token);
Expr *ast_create_array_access_expr(Arena *arena, Expr *array, Expr *index, const Token *loc_token);
Expr *ast_create_sized_array_alloc_expr(Arena *arena, Type *element_type, Expr *size_expr, Expr *default_value, const Token *loc_token);
Expr *ast_create_increment_expr(Arena *arena, Expr *operand, const Token *loc_token);
Expr *ast_create_decrement_expr(Arena *arena, Expr *operand, const Token *loc_token);
Expr *ast_create_interpolated_expr(Arena *arena, Expr **parts, char **format_specs, int part_count, const Token *loc_token);
Expr *ast_create_member_expr(Arena *arena, Expr *object, Token member_name, const Token *loc_token);
Expr *ast_create_comparison_expr(Arena *arena, Expr *left, Expr *right, SnTokenType comparison_type, const Token *loc_token);
Expr *ast_create_array_slice_expr(Arena *arena, Expr *array, Expr *start, Expr *end, Expr *step, const Token *loc_token);
Expr *ast_create_range_expr(Arena *arena, Expr *start, Expr *end, const Token *loc_token);
Expr *ast_create_spread_expr(Arena *arena, Expr *array, const Token *loc_token);
Expr *ast_create_static_call_expr(Arena *arena, Token type_name, Token method_name,
                                   Expr **arguments, int arg_count, const Token *loc_token);
Expr *ast_create_index_assign_expr(Arena *arena, Expr *array, Expr *index, Expr *value, const Token *loc_token);
Expr *ast_create_as_val_expr(Arena *arena, Expr *operand, const Token *loc_token);
Expr *ast_create_typeof_expr(Arena *arena, Expr *operand, Type *type_literal, const Token *loc_token);
Expr *ast_create_is_expr(Arena *arena, Expr *operand, Type *check_type, const Token *loc_token);
Expr *ast_create_as_type_expr(Arena *arena, Expr *operand, Type *target_type, const Token *loc_token);
Expr *ast_create_struct_literal_expr(Arena *arena, Token struct_name, FieldInitializer *fields,
                                      int field_count, const Token *loc_token);
Expr *ast_create_member_access_expr(Arena *arena, Expr *object, Token field_name, const Token *loc_token);
Expr *ast_create_member_assign_expr(Arena *arena, Expr *object, Token field_name, Expr *value, const Token *loc_token);
Expr *ast_create_sizeof_type_expr(Arena *arena, Type *type_operand, const Token *loc_token);
Expr *ast_create_sizeof_expr_expr(Arena *arena, Expr *expr_operand, const Token *loc_token);
Expr *ast_create_compound_assign_expr(Arena *arena, Expr *target, SnTokenType operator, Expr *value, const Token *loc_token);

#endif /* AST_EXPR_H */
