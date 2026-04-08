#include "type_checker.h"
#include "type_checker/util/type_checker_util.h"
#include "type_checker/stmt/type_checker_stmt.h"
#include "type_checker/type_checker_generics.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>

/* Detect cross-file struct name collisions.
 *
 * The C codegen mangles each struct to __sn__<Name> without a module qualifier,
 * so two structs declared in different source files with the same Sindarin name
 * would collide in the generated sn_types.h and fail to compile with a gcc
 * redefinition error. Catch the collision here and surface it as a clear
 * diagnostic at the type-check phase instead.
 *
 * Generic templates (type_param_count > 0) are skipped because their
 * instantiations are name-mangled per type argument (e.g. List_int) and
 * cannot collide.
 */
static void detect_duplicate_struct_decls(Module *module)
{
    for (int i = 0; i < module->count; i++)
    {
        Stmt *first = module->statements[i];
        if (first == NULL || first->type != STMT_STRUCT_DECL) continue;
        if (first->as.struct_decl.type_param_count > 0) continue;

        for (int j = i + 1; j < module->count; j++)
        {
            Stmt *second = module->statements[j];
            if (second == NULL || second->type != STMT_STRUCT_DECL) continue;
            if (second->as.struct_decl.type_param_count > 0) continue;

            if (first->as.struct_decl.name.length != second->as.struct_decl.name.length)
                continue;
            if (memcmp(first->as.struct_decl.name.start,
                       second->as.struct_decl.name.start,
                       first->as.struct_decl.name.length) != 0)
                continue;

            const char *fn1 = first->as.struct_decl.name.filename;
            const char *fn2 = second->as.struct_decl.name.filename;
            if (fn1 == NULL) fn1 = "<unknown>";
            if (fn2 == NULL) fn2 = "<unknown>";

            /* Same source file — not a cross-file collision. Leave it to the
             * parser's own duplicate-decl handling. */
            if (strcmp(fn1, fn2) == 0) continue;

            char sname[256];
            int slen = first->as.struct_decl.name.length < 255
                       ? first->as.struct_decl.name.length : 255;
            memcpy(sname, first->as.struct_decl.name.start, slen);
            sname[slen] = '\0';

            char msg[1024];
            snprintf(msg, sizeof(msg),
                     "duplicate struct '%s' defined in '%s' and '%s'",
                     sname, fn1, fn2);
            type_error(&second->as.struct_decl.name, msg);
        }
    }
}

int type_check_module(Module *module, SymbolTable *table)
{
    DEBUG_VERBOSE("Starting type checking for module with %d statements", module->count);
    type_checker_reset_error();
    generic_registry_clear();

    /* Pre-pass: catch cross-file struct name collisions before codegen would
     * otherwise emit conflicting __sn__<Name> definitions into sn_types.h. */
    detect_duplicate_struct_decls(module);
    if (type_checker_had_error())
    {
        DEBUG_VERBOSE("Type checking aborted: duplicate struct declarations detected");
        return 0;
    }

    for (int i = 0; i < module->count; i++)
    {
        type_check_stmt(module->statements[i], table, NULL);
    }
    DEBUG_VERBOSE("Type checking completed, had_type_error: %d", type_checker_had_error());
    return !type_checker_had_error();
}
