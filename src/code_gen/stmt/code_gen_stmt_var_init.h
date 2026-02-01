/*
 * code_gen_stmt_var_init.h - Variable initialization helpers
 */

#ifndef CODE_GEN_STMT_VAR_INIT_H
#define CODE_GEN_STMT_VAR_INIT_H

#include "code_gen.h"
#include "ast.h"

/* Array-to-any conversion helpers */
char *code_gen_array_to_any_1d(CodeGen *gen, Type *src_elem, const char *init_str);
char *code_gen_array_to_any_2d(CodeGen *gen, Type *inner_src, const char *init_str);
char *code_gen_array_to_any_3d(CodeGen *gen, Type *innermost_src, const char *init_str);

/* Check if an array type has 'any' as element type */
bool is_any_element_array_type(Type *type);

/* Main array conversion for var declarations */
char *code_gen_var_array_conversion(CodeGen *gen, Type *decl_type, Type *src_type, char *init_str);

#endif /* CODE_GEN_STMT_VAR_INIT_H */
