package main

/*
#include "native_array_bridge.h"
SnAbiStatus sn_test_go_typed_array_reenter(SnAbiValue *value, uintptr_t context);
*/
import "C"
import (
	"runtime"
	"runtime/cgo"
)

type typedArrayState struct { calls int; consume bool }

//export sn_test_go_typed_array_observe
func sn_test_go_typed_array_observe(view *C.SnAbiValue, context C.uintptr_t) C.SnAbiStatus {
	state := cgo.Handle(context).Value().(*typedArrayState)
	check(C.sn_test_typed_array_length(view) == C.uint64_t(state.calls+2))
	if state.consume && state.calls == 0 { C.sn_abi_v1_release(view) }
	state.calls++
	runtime.GC()
	return 0
}

//export sn_test_go_typed_arrays
func sn_test_go_typed_arrays() {
	before := C.sn_test_typed_array_destroyed()
	shape := C.SnAbiNativeArrayType{leaf_kind:C.SN_ABI_ARRAY_STRING, rank:2}
	original := C.sn_test_typed_array_new()
	var view, copied *C.SnAbiValue
	check(C.sn_abi_v1_native_array_borrow(original, shape, &view) == 0)
	alias := C.sn_abi_v1_retain(view)
	var header *C.struct_SnArray
	check(C.sn_abi_v1_native_array_data(alias, shape, &header) == 0 && header == original)
	check(C.sn_abi_v1_native_array_copy(view, &copied) == 0)
	state := &typedArrayState{}
	handle := cgo.NewHandle(state)
	check(C.sn_test_go_typed_array_reenter(view, C.uintptr_t(handle)) == 0 && state.calls == 64)
	handle.Delete()
	check(C.sn_test_typed_array_length(alias) == 65)
	C.sn_abi_v1_release(view); C.sn_abi_v1_release(alias)
	check(C.sn_test_typed_array_destroyed() == before)
	C.sn_test_typed_array_free(original)
	check(C.sn_test_typed_array_destroyed() == before+65 && C.sn_test_typed_array_length(copied) == 1)
	check(C.sn_abi_v1_native_array_take(&copied, shape, &header) == 0 && copied == nil)
	C.sn_test_typed_array_free(header)
	check(C.sn_test_typed_array_destroyed() == before+66)
	check(C.sn_abi_v1_native_array_adopt(C.sn_test_typed_array_new(), shape, &view) == 0)
	state = &typedArrayState{consume:true}; handle = cgo.NewHandle(state)
	check(C.sn_test_go_typed_array_reenter(view, C.uintptr_t(handle)) == 0 && state.calls == 64)
	handle.Delete()
	check(C.sn_test_typed_array_destroyed() == before+131)
}
