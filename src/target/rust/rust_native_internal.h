#ifndef SN_RUST_NATIVE_INTERNAL_H
#define SN_RUST_NATIVE_INTERNAL_H

#include "target/rust/rust_native.h"
#include "cgen/gen_model_split.h"

ModularModel *rust_native_plan_split(RustNativePlan *plan);
json_object *rust_native_plan_handles(RustNativePlan *plan);
json_object *rust_native_plan_array_support(RustNativePlan *plan);
json_object *rust_native_plan_record_support(RustNativePlan *plan);
json_object *rust_native_plan_callback_support(RustNativePlan *plan);
json_object *rust_native_plan_interface_support(RustNativePlan *plan);
json_object *rust_native_plan_declaration_support(RustNativePlan *plan);
bool rust_native_plan_set_interface_support(RustNativePlan *plan, json_object *support);
bool rust_native_impl_has_callable_body(json_object *impl);

#endif
