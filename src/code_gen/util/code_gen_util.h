#ifndef CODE_GEN_UTIL_H
#define CODE_GEN_UTIL_H

#include "arena.h"
#include "ast.h"
#include "token.h"
#include "code_gen.h"
#include <stdarg.h>
#include <stdbool.h>

/* Arena-based string formatting */
char *arena_vsprintf(Arena *arena, const char *fmt, va_list args);
char *arena_sprintf(Arena *arena, const char *fmt, ...);

/* C code escaping */
char *escape_char_literal(Arena *arena, char ch);
char *escape_c_string(Arena *arena, const char *str);

/* Type conversion helpers */
const char *get_c_type(Arena *arena, Type *type);
const char *get_c_param_type(Arena *arena, Type *type);  /* C types for function params (RtHandle for str/arr) */
const char *get_c_native_param_type(Arena *arena, Type *type);  /* C types for native function params (const char * for str) */
const char *get_c_array_elem_type(Arena *arena, Type *elem_type);  /* Storage type for array elements */
const char *get_c_sizeof_elem(Arena *arena, Type *elem_type);  /* sizeof(elem_type) expression for codegen */
bool is_handle_type(Type *type);  /* True if type uses RtHandle storage */
const char *get_rt_to_string_func(TypeKind kind);
const char *get_rt_to_string_func_v2(TypeKind kind);  /* V2 raw pointer version */
const char *get_rt_to_string_func_for_type(Type *type);
const char *get_rt_to_string_func_for_type_v2(Type *type);  /* V2 version for arena mode */
const char *get_default_value(Type *type);
const char *get_rt_result_type(Type *type);

/* Any type boxing helpers */
const char *get_boxing_function(Type *type);
const char *get_unboxing_function(Type *type);
const char *get_element_type_tag(Type *element_type);
int get_struct_type_id(Type *struct_type);
char *code_gen_box_value(CodeGen *gen, const char *value_str, Type *value_type);
char *code_gen_unbox_value(CodeGen *gen, const char *any_str, Type *target_type);

/* Struct type resolution for code gen */
Type *resolve_struct_type(CodeGen *gen, Type *type);

/* Name helpers */
char *get_var_name(Arena *arena, Token name);
char *sn_mangle_name(Arena *arena, const char *name);

/* Code generation helpers */
void indented_fprintf(CodeGen *gen, int indent, const char *fmt, ...);
char *code_gen_binary_op_str(SnTokenType op);
char *code_gen_type_suffix(Type *type);

/* Helper macro to get arena var for formatting - returns "NULL" if no arena context */
#define ARENA_VAR(gen) ((gen)->current_arena_var ? (gen)->current_arena_var : "NULL")

/* Constant folding optimization */

/* Check if an expression is a compile-time constant (literal or constant expression) */
bool is_constant_expr(Expr *expr);

/* Try to evaluate a constant expression. Returns true if successful, false otherwise.
   For integers, result is stored in out_int_value.
   For doubles, result is stored in out_double_value.
   out_is_double is set to true if the result is a double, false if integer. */
bool try_fold_constant(Expr *expr, int64_t *out_int_value, double *out_double_value, bool *out_is_double);

/* Generate code for a constant-folded expression, returning the literal string.
   Returns NULL if the expression cannot be folded. */
char *try_constant_fold_binary(CodeGen *gen, BinaryExpr *expr);
char *try_constant_fold_unary(CodeGen *gen, UnaryExpr *expr);

/* Native C operator helpers for unchecked arithmetic mode */

/* Get the native C operator string for a token type.
   Returns NULL if the operator doesn't have a direct C equivalent. */
const char *get_native_c_operator(SnTokenType op);

/* Check if an operator can use native C operators in unchecked mode.
   Division and modulo still need runtime functions for zero-check. */
bool can_use_native_operator(SnTokenType op);

/* Generate native C arithmetic expression for unchecked mode.
   Returns the expression string, or NULL if runtime function is required. */
char *gen_native_arithmetic(CodeGen *gen, const char *left_str, const char *right_str,
                            SnTokenType op, Type *type);

/* Generate native C unary expression for unchecked mode.
   Returns the expression string, or NULL if runtime function is required. */
char *gen_native_unary(CodeGen *gen, const char *operand_str, SnTokenType op, Type *type);

/* Array element accessor suffix for typed get/set functions.
   Returns the suffix for rt_array_get_<suffix>_v2 / rt_array_set_<suffix>_v2.
   Returns NULL for struct/complex types that need rt_array_data_begin_v2. */
const char *get_array_accessor_suffix(Type *elem_type);

/* Struct field promotion for thread sync and function returns */

/* Generate code to promote all handle fields in a struct from src_arena to dest_arena.
   This handles strings, arrays (1D, 2D, 3D), and nested types.
   Returns code that promotes var_name's fields in place.
   The generated code assumes var_name is already assigned with raw values. */
char *gen_struct_field_promotion(CodeGen *gen, Type *struct_type, const char *var_name,
                                  const char *dest_arena, const char *src_arena);

/* Check if a struct type has any handle fields that need promotion */
bool struct_has_handle_fields(Type *struct_type);

/* GC callback generation for struct types.
 * Generates copy/free callbacks for structs with handle fields so that
 * rt_arena_v2_promote() and GC sweep handle deep copy/free automatically. */
void code_gen_ensure_struct_callbacks(CodeGen *gen, Type *struct_type);

/* Arena destination calculation for scope escape */

/* Calculate the target arena for an escaping allocation based on scope depth.
   When a struct or allocation escapes from an inner scope (source_depth) to an
   outer scope (target_depth), this function determines which arena to allocate
   in to ensure the value lives long enough.

   Parameters:
   - gen: The code generator state containing arena stack
   - source_depth: The current scope depth where allocation occurs
   - target_depth: The destination scope depth where value escapes to

   Returns the arena variable name to use for allocation, or the current arena
   if no escape is needed. Returns "NULL" if escaping to global scope. */
const char *get_arena_for_scope_escape(CodeGen *gen, int source_depth, int target_depth);

/* Calculate the number of arena levels to traverse in parent chain.
   Returns the difference between current arena depth and target depth.
   Used for generating rt_arena_get_parent() chains. */
int calculate_arena_traversal_depth(CodeGen *gen, int source_depth, int target_depth);

/* Get the arena at a specific depth in the arena stack.
   Returns NULL if the depth is out of range. */
const char *get_arena_at_depth(CodeGen *gen, int depth);

/* Arena requirement analysis */

/* Check if a function body needs arena allocation.
   Returns true if the function uses strings, arrays, or other heap-allocated types.
   Returns false if the function only uses primitives (int, double, bool, char). */
bool function_needs_arena(FunctionStmt *fn);

/* Check if an expression requires arena allocation */
bool expr_needs_arena(Expr *expr);

/* Check if a statement requires arena allocation */
bool stmt_needs_arena(Stmt *stmt);

/* Tail call optimization helpers */

/* Check if a function has any tail calls marked for optimization */
bool function_has_marked_tail_calls(FunctionStmt *fn);

/* Check if a statement contains any marked tail calls */
bool stmt_has_marked_tail_calls(Stmt *stmt);

/* Arena temp handle tracking */

/* Emit a pre-declaration for a temp handle and track it for later cleanup.
 * Returns the temp variable name (e.g. "__htmp_0__"). */
char *code_gen_emit_arena_temp(CodeGen *gen, const char *expr_str);

/* Free all tracked arena temps after a statement completes. */
void code_gen_flush_arena_temps(CodeGen *gen, int indent);

/* Remove temps from saved_count onwards — adopted by a consumer (var decl, return, assign). */
void code_gen_adopt_arena_temps_from(CodeGen *gen, int saved_count);

#endif /* CODE_GEN_UTIL_H */
