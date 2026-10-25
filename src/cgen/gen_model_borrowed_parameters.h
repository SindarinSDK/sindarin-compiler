#ifndef GEN_MODEL_BORROWED_PARAMETERS_H
#define GEN_MODEL_BORROWED_PARAMETERS_H

#include "ast.h"
#include <json-c/json.h>

/* C ownership annotations shared by the C and private native-body projections. */
typedef struct { json_object *parameters; int id; } GenModelBorrowedParamContext;
GenModelBorrowedParamContext gen_model_borrowed_parameters_enter(json_object *parameters);
void gen_model_borrowed_parameters_restore(GenModelBorrowedParamContext context);
void gen_model_borrowed_parameter_mutation(json_object *model, Expr *expr);

#endif
