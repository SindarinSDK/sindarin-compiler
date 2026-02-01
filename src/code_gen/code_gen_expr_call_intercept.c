/**
 * code_gen_expr_call_intercept.c - Code generation for intercepted calls
 *
 * Contains functions for generating intercepted function and method calls.
 * Interception allows middleware-style wrapping of function calls at runtime.
 */

#include "code_gen/code_gen_expr_call.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* Include split modules */
#include "code_gen_expr_call_intercept_func.c"
#include "code_gen_expr_call_intercept_method.c"

