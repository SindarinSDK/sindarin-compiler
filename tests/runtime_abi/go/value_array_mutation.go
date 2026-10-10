package main

/* #include <stdlib.h>
#include "sn_abi.h"
SnAbiStatus sn_test_go_resource(uintptr_t context, SnAbiValue **out);
SnAbiStatus sn_test_go_mutation_resource(uintptr_t context, SnAbiValue **out); */
import "C"
import (
	"runtime"
	"runtime/cgo"
	"unsafe"
)

type mutationState struct {
	array  *C.SnAbiValue
	marker *C.SnAbiValue
	calls  int
}

//export sn_test_go_mutation_destroy
func sn_test_go_mutation_destroy(data unsafe.Pointer, context C.uintptr_t) {
	check(data == nil)
	handle := cgo.Handle(context)
	state := handle.Value().(*mutationState)
	var length C.uint64_t
	check(C.sn_abi_v1_value_array_length(state.array, &length) == 0 && length == 0)
	C.sn_abi_v1_release(state.array)
	runtime.GC()
	for i := 0; i < 64; i++ {
		check(C.sn_abi_v1_value_array_insert(state.array, 0, nil) == 0)
	}
	check(C.sn_abi_v1_value_array_reverse(state.array) == 0)
	check(C.sn_abi_v1_value_array_clear(state.array) == 0)
	check(C.sn_abi_v1_value_array_insert(state.array, 0, state.marker) == 0)
	state.calls++
	handle.Delete()
}

func checkArrayMutation() {
	var info C.SnAbiInfo
	versions := []C.uint32_t{C.SN_ABI_V1_VERSION, C.SN_ABI_V1_1_VERSION, C.SN_ABI_V1_2_VERSION, C.SN_ABI_V1_3_VERSION}
	masks := []C.uint64_t{7, 31, 63, 127}
	for i, version := range versions {
		check(C.sn_abi_v1_query(version, 0, &info, C.sizeof_SnAbiInfo) == 0)
		check(info.abi_version == version && info.capabilities == masks[i])
		check(C.sn_abi_v1_query(version, C.SN_ABI_CAP_ARRAY_MUTATION, &info, C.sizeof_SnAbiInfo) == C.SN_ABI_UNSUPPORTED)
		check(info.abi_version == version && info.capabilities == masks[i])
	}
	check(C.sn_abi_v1_query(C.SN_ABI_V1_4_VERSION, C.SN_ABI_CAP_ARRAY_MUTATION, &info, C.sizeof_SnAbiInfo) == 0)
	check(info.abi_version == C.SN_ABI_V1_4_VERSION && info.capabilities == 255)
	var array, text, resource, copy, out *C.SnAbiValue
	bytes := C.CString(string([]byte{128, 255}))
	check(C.sn_abi_v1_string_copy(bytes, &text) == 0)
	C.free(unsafe.Pointer(bytes))
	check(C.sn_abi_v1_value_array_new(&array) == 0)
	out = text
	check(C.sn_abi_v1_value_array_pop(array, &out) == C.SN_ABI_OUT_OF_RANGE && out == text)
	check(C.sn_abi_v1_value_array_take(text, 0, &out) == C.SN_ABI_WRONG_KIND && out == text)
	check(C.sn_abi_v1_value_array_insert(array, ^C.uint64_t(0), text) == C.SN_ABI_OUT_OF_RANGE)
	check(C.sn_abi_v1_value_array_insert(array, 0, text) == 0)
	check(C.sn_abi_v1_value_array_insert(array, 0, nil) == 0)
	alias := C.sn_abi_v1_retain(array)
	check(C.sn_abi_v1_value_array_reverse(array) == 0)
	check(C.sn_abi_v1_value_array_pop(alias, &out) == 0 && out == nil)
	check(C.sn_abi_v1_value_array_take(array, 0, &out) == 0 && out == text)
	C.sn_abi_v1_release(array)
	C.sn_abi_v1_release(alias)
	runtime.GC()
	var view C.SnAbiBytes
	check(C.sn_abi_v1_string_bytes(out, &view) == 0)
	check(string(C.GoBytes(unsafe.Pointer(view.data), C.int(view.length))) == string([]byte{128, 255}))
	C.sn_abi_v1_release(out)

	check(C.sn_abi_v1_value_array_new(&array) == 0)
	state := &resourceState{}
	handle := cgo.NewHandle(state)
	check(C.sn_test_go_resource(C.uintptr_t(handle), &resource) == 0)
	check(C.sn_abi_v1_value_array_insert(array, 0, resource) == 0)
	C.sn_abi_v1_release(resource)
	check(C.sn_abi_v1_value_array_copy(array, &copy) == 0)
	check(C.sn_abi_v1_value_array_remove(array, 0) == 0 && state.destroyed == 0)
	check(C.sn_abi_v1_value_array_pop(copy, &out) == 0 && out == resource)
	C.sn_abi_v1_release(copy)
	C.sn_abi_v1_release(array)
	runtime.GC()
	check(state.destroyed == 0)
	C.sn_abi_v1_release(out)
	check(state.destroyed == 1 && state.value == 42)

	for _, clear := range []bool{false, true} {
		check(C.sn_abi_v1_value_array_new(&array) == 0)
		state := &mutationState{array: array, marker: text}
		handle := cgo.NewHandle(state)
		check(C.sn_test_go_mutation_resource(C.uintptr_t(handle), &resource) == 0)
		check(C.sn_abi_v1_value_array_insert(array, 0, resource) == 0)
		C.sn_abi_v1_release(resource)
		if clear {
			check(C.sn_abi_v1_value_array_clear(array) == 0)
		} else {
			check(C.sn_abi_v1_value_array_remove(array, 0) == 0)
		}
		check(state.calls == 1)
	}
	C.sn_abi_v1_release(text)
}

//export sn_test_go_array_mutation
func sn_test_go_array_mutation() C.uint32_t {
	checkArrayMutation()
	return 0
}
