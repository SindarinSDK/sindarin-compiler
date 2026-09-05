#ifndef SN_RUST_NATIVE_INTERNAL_H
#define SN_RUST_NATIVE_INTERNAL_H

#include "target/rust/rust_native.h"
#include "cgen/gen_model_split.h"

ModularModel *rust_native_plan_split(RustNativePlan *plan);

#endif
