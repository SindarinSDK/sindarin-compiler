#include "type_checker.h"
#include "type_checker/type_checker_util.h"
#include "type_checker/type_checker_stmt.h"
#include "debug.h"

int type_check_module(Module *module, SymbolTable *table)
{
    DEBUG_VERBOSE("Starting type checking for module with %d statements", module->count);
    type_checker_reset_error();
    for (int i = 0; i < module->count; i++)
    {
        type_check_stmt(module->statements[i], table, NULL);
    }
    DEBUG_VERBOSE("Type checking completed, had_type_error: %d", type_checker_had_error());
    return !type_checker_had_error();
}
