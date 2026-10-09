#ifndef SN_PACKAGE_IMPORT_H
#define SN_PACKAGE_IMPORT_H
#include "../compiler.h"
bool package_prepare_native_imports(CompilerOptions *options, Module *module,
                                   const char *const *paths, Module *const *modules, int count);
#endif
