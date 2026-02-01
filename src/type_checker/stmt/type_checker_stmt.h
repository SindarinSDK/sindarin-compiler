#ifndef TYPE_CHECKER_STMT_H
#define TYPE_CHECKER_STMT_H

#include "ast.h"
#include "symbol_table.h"

/* Statement type checking */
void type_check_stmt(Stmt *stmt, SymbolTable *table, Type *return_type);

#endif /* TYPE_CHECKER_STMT_H */
