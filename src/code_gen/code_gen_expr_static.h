#ifndef CODE_GEN_EXPR_STATIC_H
#define CODE_GEN_EXPR_STATIC_H

#include "code_gen.h"

/* Static call expression code generation for built-in types:
 * - Stdin static methods (readLine, readChar, readWord, hasChars, hasLines, isEof)
 * - Stdout static methods (write, writeLine, flush)
 * - Stderr static methods (write, writeLine, flush)
 * - Interceptor static methods (register, registerWhere, clearAll, isActive, count)
 * - User-defined struct static methods
 */
char *code_gen_static_call_expression(CodeGen *gen, Expr *expr);

/* Method call expression code generation (EXPR_METHOD_CALL):
 * Handles both static and instance method calls on user-defined structs.
 * The method and struct_type are already resolved by the type checker.
 */
char *code_gen_method_call_expression(CodeGen *gen, Expr *expr);

#endif /* CODE_GEN_EXPR_STATIC_H */
