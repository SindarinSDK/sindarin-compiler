/**
 * code_gen_expr_call_array.c - Code generation for array method calls
 *
 * Handles push, clear, pop and other array methods.
 */

#include "code_gen/code_gen_expr_call.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
#include "symbol_table.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Include split modules */
#include "code_gen_expr_call_array_push.c"
#include "code_gen_expr_call_array_query.c"
#include "code_gen_expr_call_array_mutate.c"
#include "code_gen_expr_call_array_byte.c"

