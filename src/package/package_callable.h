#ifndef SN_PACKAGE_CALLABLE_H
#define SN_PACKAGE_CALLABLE_H
#include "../compiler.h"
#include <json-c/json.h>

typedef struct {
    FunctionStmt *function;
    StructMethod *method;
    StructDeclStmt *owner;
    Type *self_type;
} PackageCallable;

bool package_find_callable(Arena *arena, Module *module, const char *file,
                            const char *name, PackageCallable *out);
bool package_callable_valid(const PackageCallable *callable, bool sindarin);
json_object *package_callable_signature(CompilerOptions *options, const PackageCallable *callable,
                                       json_object *plan, json_object *binding, Module *module,
                                       Module *const *modules, int count);
void package_callable_adapt(CompilerOptions *options, const PackageCallable *callable,
                            const char *alias);
bool package_binding_is_sindarin(json_object *plan, json_object *binding);
#endif
