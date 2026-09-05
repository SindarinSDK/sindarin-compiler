# Rust validator extraction movement inventory

Original movement-comparison base: `7249e7c1c3ff57c7f8f72abc488cb79754a0404a`. PR #113 landed at `3f6d23c0d609a63865ea58f98e81f1951dba816b`; this extraction is now rebased onto that exact `origin/main`. The complete `src` and `templates` trees are byte-identical to pre-rebase extraction head `72cc6e81c6af013b265edeb81b4eac1d4b24b035`, verified by `git diff --exit-code` over those paths. This inventory records the behavior-preserving extraction, not feature completion.

The comparison enumerated every original static function definition by name and compared its complete signature/body bytes against the new private fragments. All 101 original functions remain: 96 bodies compare byte-for-byte. Inlining the new hook calls reconstructs the remaining five original bodies byte-for-byte. Original declarations, scope structs, enums, static validation state and comments move with their owning fragment; include order preserves helper availability. No linked/public symbol is introduced.

| Changed existing function | Exact glue |
|---|---|
| `rust_type_supported` | The existing rejected `function` kind calls `rust_closure_type_supported`, which returns false. All other type paths are unchanged. |
| `rust_validate_expr` | Static/ordinary call branches invoke their extracted bodies at the same dispatch positions. Variable admission invokes an always-true hook. Previously unsupported lambda/resolved-call kinds invoke always-false hooks at the fallback. |
| `rust_validate_model_impl` | The existing lambda-array gate invokes `rust_validate_closures`, which uses the identical `array_is_empty(model, "lambdas")` predicate. |
| `rust_lower_strings` | The call/static-call annotation branch invokes `rust_lower_call_strings` at its original position, after child traversal and earlier match/string exits. |
| `rust_emit` | Adds no-op closure lowering after validation. Groups the existing array-search then instance-clone passes in `rust_lower_calls` without changing their order relative to any other pass. |

`rust_validate_call` consults the closure-call hook first; its current UNHANDLED result always continues the intact ordinary call body. No diagnostic is emitted or suppressed by a new hook. Function-type rendering adds an explicit hook returning the same `()` as the old fallback. All other original renderer functions are unchanged. Existing call/variable/assignment partial bytes move to `direct_call`, `direct_variable`, and `direct_assign`; the previous names delegate through closure-specific pass-through partials. Lambda, method-call, inferred-call and module-support placeholders emit zero bytes, preserving existing fallback output. The partial loader already discovers these files recursively.

The full positive/negative suites and snapshots are unchanged. See [parity ledger verification and ownership](rust-backend-parity.md#validator-extraction-prerequisite-and-next-author-boundaries-2026-09-05) for commands, counts, ordering, author boundaries and risks.

## Original function destinations

### `src/target/rust/rust_lower.c` (14)

- `rust_lower_checked_arithmetic`
- `rust_lower_checked_mutations`
- `rust_lower_floating_mutations`
- `rust_lower_strings`
- `rust_lower_interpolation_formats`
- `rust_mark_for_continues`
- `rust_lower_for_continues`
- `rust_model_uses_string_helpers`
- `rust_model_uses_string_format_helpers`
- `rust_mark_scalar_ref_uses`
- `rust_lower_scalar_ref_parameters`
- `rust_model_contains_string`
- `rust_lower_iterator_temp_names`
- `rust_lower_match_temp_names`

### `src/target/rust/rust_lower_calls.c` (4)

- `rust_lower_array_searches`
- `rust_owned_value_type`
- `rust_mark_instance_method_clones`
- `rust_lower_instance_method_clones`

### `src/target/rust/rust_target.c` (6)

- `rustc_command`
- `rustc_quoted`
- `rust_run_command`
- `rust_check_toolchain`
- `rust_emit`
- `rust_build`

### `src/target/rust/rust_validate.c` (66)

- `array_is_empty`
- `rust_type_supported`
- `json_string_property`
- `json_boolean_property`
- `json_string_property_equals`
- `rust_typeof_type_supported`
- `rust_validate_typeof_operand`
- `rust_scalar_ref_parameter_type_supported`
- `rust_by_value_assign_parameter_type_supported`
- `rust_direct_variable_named`
- `rust_rhs_mutates_or_forwards_parameter`
- `rust_find_parameter`
- `rust_name_is_shadowed`
- `rust_assignment_place_root`
- `rust_prepare_parameter_mutations_in_node`
- `rust_prepare_callable_parameter_mutations`
- `rust_prepare_by_value_scalar_parameter_mutations`
- `rust_floating_type`
- `rust_floating_ref_parameter`
- `rust_checked_scalar_ref_parameter`
- `rust_validate_structs`
- `rust_mark_iterator_binding_mutation`
- `rust_report_match_error`
- `rust_integer_type`
- `rust_float_type`
- `rust_fixed_sizeof_bytes`
- `rust_report_unsupported_sizeof`
- `rust_signed_integer_type`
- `rust_unsigned_integer_type`
- `rust_find_struct`
- `rust_reachable_user_copy_struct`
- `rust_auto_copy_plain_value_struct_type`
- `rust_array_concat_type_supported`
- `rust_heap_free_named_struct_type`
- `rust_array_copy_type_supported`
- `rust_parse_format_spec`
- `rust_validate_expr_array`
- `rust_reflection_field_is`
- `rust_reflection_schema_is_current`
- `rust_validate_expr`
- `rust_model_uses_arrays`
- `rust_model_uses_reflection`
- `rust_validate_statements`
- `rust_validate_block`
- `rust_match_integral_type`
- `rust_match_integer_types_shared_compatible`
- `rust_match_literal_model_value`
- `rust_match_positive_literal_intrinsically_reliable`
- `rust_match_positive_value_fits_subject`
- `rust_match_negative_magnitude_fits_subject`
- `rust_integral_match_literal_pattern`
- `rust_report_integral_match_pattern_error`
- `rust_bool_match_literal_pattern`
- `rust_string_match_constant_pattern_value`
- `rust_prepare_string_match_pattern`
- `rust_string_match_result_call_is_mutating`
- `rust_string_match_result_access_is_stable`
- `rust_float_match_literal_pattern`
- `rust_report_float_match_pattern_error`
- `rust_validate_statement_match`
- `rust_validate_value_match`
- `rust_iterator_scalar_element_supported`
- `rust_validate_for_each_iter`
- `rust_validate_stmt`
- `rust_validate_model_impl`
- `rust_validate_model`

### `src/target/rust/rust_validate_calls.c` (11)

- `rust_array_method_supported`
- `rust_string_method_supported`
- `rust_primitive_conversion_member`
- `rust_primitive_integer_conversion_supported`
- `rust_array_search_type_supported`
- `rust_mutation_place_is_self_rooted`
- `rust_is_mutating_array_call`
- `rust_instance_method_node_supported`
- `rust_method_has_direct_mutation`
- `rust_method_calls_mutating_self`
- `rust_validate_struct_methods`

## New private entrypoints

- Calls: `rust_validate_call`, `rust_validate_static_call`, `rust_validate_resolved_call`, `rust_lower_call_strings`, `rust_lower_calls`.
- Closures: `rust_validate_closures`, `rust_closure_type_supported`, `rust_validate_lambda`, `rust_validate_function_value`, `rust_validate_closure_call`, `rust_lower_closures`, renderer `rust_closure_type`.

These hooks select no closure representation and do not implement closure support. Common recursive validators and existing static type/model helpers remain available to both families in the one translation unit. New code must preserve diagnostic state and child traversal order.
