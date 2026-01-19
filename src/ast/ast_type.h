#ifndef AST_TYPE_H
#define AST_TYPE_H

#include "ast.h"
#include "arena.h"

/* Type creation functions */
Type *ast_clone_type(Arena *arena, Type *type);
Type *ast_create_primitive_type(Arena *arena, TypeKind kind);
Type *ast_create_array_type(Arena *arena, Type *element_type);
Type *ast_create_function_type(Arena *arena, Type *return_type, Type **param_types, int param_count);
Type *ast_create_struct_type(Arena *arena, const char *name, StructField *fields, int field_count,
                             StructMethod *methods, int method_count, bool is_native, bool is_packed,
                             bool pass_self_by_ref, const char *c_alias);
StructMethod *ast_struct_get_method(Type *struct_type, const char *method_name);

/* Type utilities */
int ast_type_equals(Type *a, Type *b);
const char *ast_type_to_string(Arena *arena, Type *type);
int ast_type_is_struct(Type *type);
StructField *ast_struct_get_field(Type *struct_type, const char *field_name);
int ast_struct_get_field_index(Type *struct_type, const char *field_name);

#endif /* AST_TYPE_H */
