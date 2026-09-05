#ifndef SN_RUST_NATIVE_H
#define SN_RUST_NATIVE_H

#include "target/target.h"
#include <json-c/json.h>
#include <stdbool.h>

typedef struct RustNativePlan RustNativePlan;

bool rust_native_partition_model(json_object *rust_model,
                                 const CompilerOptions *options,
                                 RustNativePlan **out_plan);
bool rust_native_validate_declaration(const RustNativePlan *plan,
                                      json_object *function);
bool rust_native_plan_has_work(const RustNativePlan *plan);
bool rust_native_emit_support(RustNativePlan *plan, GeneratedFileSet *files,
                              const char *compiler_dir);
bool rust_native_build(const CompilerOptions *options, const char *build_dir,
                       const GeneratedFileSet *files, RustNativePlan *plan);
void rust_native_plan_free(void *opaque);

#endif
