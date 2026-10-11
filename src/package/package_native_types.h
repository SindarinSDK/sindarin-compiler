#ifndef SN_PACKAGE_NATIVE_TYPES_H
#define SN_PACKAGE_NATIVE_TYPES_H
#include "../ast.h"
#include "../compiler.h"
#include <json-c/json.h>

bool package_native_bind_records(Arena *arena, json_object *plan, const char *manifest,
                                 Module *module);
json_object *package_native_model_type(Arena *arena, Type *type, Module *module,
                                       Module *const *modules, int count);
bool package_native_validate_records(CompilerOptions *options, json_object *plan);
#endif
