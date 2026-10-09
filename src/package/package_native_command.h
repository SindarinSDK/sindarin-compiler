#ifndef PACKAGE_NATIVE_COMMAND_H
#define PACKAGE_NATIVE_COMMAND_H
#include "../compiler.h"
int package_native_run_driver(char *const *args);
int package_native_command(CompilerOptions *options);
#endif
