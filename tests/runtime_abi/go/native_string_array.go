package main

/*
#include <stdlib.h>
#include "native_string_array_bridge.h"
SnAbiStatus sn_test_go_native_array_reenter(SnAbiValue *view, uintptr_t context);
*/
import "C"
import (
	"runtime"
	"runtime/cgo"
	"unsafe"
)

type nativeArrayState struct {
	alias   *C.SnAbiValue
	calls   int
	consume bool
}

//export sn_test_go_native_array_observe
func sn_test_go_native_array_observe(view *C.SnAbiValue, context C.uintptr_t) C.SnAbiStatus {
	state := cgo.Handle(context).Value().(*nativeArrayState)
	runtime.GC()
	var length C.uint64_t
	if C.sn_test_native_strings_length(state.alias, &length) != 0 || uint64(length) != uint64(5+2*state.calls) {
		return C.SN_ABI_FOREIGN_ERROR
	}
	if state.consume && state.calls == 0 {
		C.sn_abi_v1_release(view)
	}
	state.calls++
	text := C.CString("Go callback")
	defer C.free(unsafe.Pointer(text))
	return C.sn_test_native_strings_push(view, text)
}

func checkNativeStringArrays() {
	var info C.SnAbiInfo
	check(C.sn_abi_v1_query(C.SN_ABI_V1_5_VERSION, C.SN_ABI_CAP_NATIVE_STRING_ARRAYS, &info, C.sizeof_SnAbiInfo) == 0 && info.capabilities == 511)
	before := C.sn_test_native_strings_destroyed()
	native := C.sn_test_native_strings_new()
	var view, copied *C.SnAbiValue
	check(C.sn_abi_v1_native_string_array_borrow(native, &view) == 0)
	alias := C.sn_abi_v1_retain(view)
	var pointer *C.struct_SnArray
	check(C.sn_abi_v1_native_string_array_data(alias, &pointer) == 0 && pointer == native)
	check(C.sn_abi_v1_native_string_array_copy(view, &copied) == 0)
	state := &nativeArrayState{alias: alias}
	handle := cgo.NewHandle(state)
	check(C.sn_test_go_native_array_reenter(view, C.uintptr_t(handle)) == 0 && state.calls == 64)
	handle.Delete()
	var length C.uint64_t
	check(C.sn_test_native_strings_length(alias, &length) == 0 && length == 132)
	check(C.sn_test_native_strings_length(copied, &length) == 0 && length == 4)
	var items [4]*C.SnAbiValue
	for i := range items {
		check(C.sn_test_native_strings_read(copied, C.uint64_t(i), &items[i]) == 0)
	}
	C.sn_abi_v1_release(view)
	C.sn_abi_v1_release(alias)
	check(C.sn_test_native_strings_destroyed() == before)
	C.sn_test_native_strings_free(native)
	C.sn_abi_v1_release(copied)
	check(C.sn_test_native_strings_destroyed() == before+136)
	wanted := []string{"one", "", "", string([]byte{128, 255})}
	for i, item := range items {
		var bytes C.SnAbiBytes
		check(C.sn_abi_v1_string_bytes(item, &bytes) == 0)
		check((bytes.data == nil) == (i == 1))
		check(string(C.GoBytes(unsafe.Pointer(bytes.data), C.int(bytes.length))) == wanted[i])
		C.sn_abi_v1_release(item)
	}
	check(C.sn_abi_v1_native_string_array_adopt(C.sn_test_native_strings_new(), &view) == 0)
	state = &nativeArrayState{alias: view, consume: true}
	handle = cgo.NewHandle(state)
	check(C.sn_test_go_native_array_reenter(view, C.uintptr_t(handle)) == 0 && state.calls == 64)
	handle.Delete()
	check(C.sn_test_native_strings_destroyed() == before+268)
}

//export sn_test_go_native_arrays
func sn_test_go_native_arrays() C.uint32_t { checkNativeStringArrays(); return 0 }
