package main

/* #include <stdlib.h>
#include "sn_abi.h"
SnAbiStatus sn_test_go_resource(uintptr_t handle, SnAbiValue **out); */
import "C"
import (
	"runtime"
	"runtime/cgo"
	"unsafe"
)

func checkArrayAssignment() {
	var info C.SnAbiInfo
	versions := []C.uint32_t{C.SN_ABI_V1_VERSION, C.SN_ABI_V1_1_VERSION, C.SN_ABI_V1_2_VERSION}
	masks := []C.uint64_t{7, 31, 63}
	for i, version := range versions {
		check(C.sn_abi_v1_query(version, 0, &info, C.sizeof_SnAbiInfo) == 0)
		check(info.abi_version == version && info.capabilities == masks[i])
		check(C.sn_abi_v1_query(version, C.SN_ABI_CAP_ARRAY_REPLACEMENT, &info, C.sizeof_SnAbiInfo) == C.SN_ABI_UNSUPPORTED)
		check(info.abi_version == version && info.capabilities == masks[i])
	}
	check(C.sn_abi_v1_query(C.SN_ABI_V1_3_VERSION, C.SN_ABI_CAP_ARRAY_REPLACEMENT, &info, C.sizeof_SnAbiInfo) == 0)
	check(info.abi_version == C.SN_ABI_V1_3_VERSION && info.capabilities == 127)
	var source, destination, resource, text *C.SnAbiValue
	check(C.sn_abi_v1_value_array_new(&source) == 0)
	check(C.sn_abi_v1_value_array_new(&destination) == 0)
	state := &resourceState{}
	handle := cgo.NewHandle(state)
	check(C.sn_test_go_resource(C.uintptr_t(handle), &resource) == 0)
	check(C.sn_abi_v1_value_array_push(source, resource) == 0)
	check(C.sn_abi_v1_value_array_push(destination, resource) == 0)
	C.sn_abi_v1_release(resource)
	bytes := C.CString(string([]byte{128, 255}))
	check(C.sn_abi_v1_string_copy(bytes, &text) == 0)
	C.free(unsafe.Pointer(bytes))
	check(C.sn_abi_v1_value_array_push(source, text) == 0)
	check(C.sn_abi_v1_value_array_push(source, nil) == 0)
	alias := C.sn_abi_v1_retain(destination)
	check(C.sn_abi_v1_value_array_assign(destination, text) == C.SN_ABI_WRONG_KIND)
	var length C.uint64_t
	check(C.sn_abi_v1_value_array_length(alias, &length) == 0 && length == 1)
	check(state.destroyed == 0)
	check(C.sn_abi_v1_value_array_assign(destination, source) == 0)
	check(C.sn_abi_v1_value_array_assign(destination, alias) == 0)
	C.sn_abi_v1_release(source)
	C.sn_abi_v1_release(destination)
	runtime.GC()
	check(C.sn_abi_v1_value_array_length(alias, &length) == 0 && length == 3)
	check(C.sn_abi_v1_value_array_get(alias, 0, &resource) == 0)
	check(C.sn_abi_v1_value_array_assign(alias, nil) == 0)
	check(C.sn_abi_v1_value_array_length(alias, &length) == 0 && length == 0)
	check(state.destroyed == 0)
	check(C.sn_abi_v1_value_array_assign(nil, alias) == C.SN_ABI_INVALID_ARGUMENT)
	C.sn_abi_v1_release(alias)
	C.sn_abi_v1_release(resource)
	check(state.destroyed == 1 && state.value == 42)
	var view C.SnAbiBytes
	check(C.sn_abi_v1_string_bytes(text, &view) == 0)
	check(string(C.GoBytes(unsafe.Pointer(view.data), C.int(view.length))) == string([]byte{128, 255}))
	C.sn_abi_v1_release(text)
}

//export sn_test_go_array_assignment
func sn_test_go_array_assignment() C.uint32_t {
	checkArrayAssignment()
	return 0
}
