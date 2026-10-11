package main

/*
#include "sn_array.h"
#include "native_byte_array_bridge.h"
SnAbiStatus sn_test_go_native_bytes_reenter(SnAbiValue *value, uintptr_t context);
*/
import "C"
import (
	"bytes"
	"runtime"
	"runtime/cgo"
	"unsafe"
)

type byteArrayState struct {
	calls   int
	consume bool
}

//export sn_test_go_native_bytes_observe
func sn_test_go_native_bytes_observe(view *C.SnAbiValue, context C.uintptr_t) C.SnAbiStatus {
	state := cgo.Handle(context).Value().(*byteArrayState)
	var data C.SnAbiBytes
	if C.sn_abi_v1_native_byte_array_bytes(view, &data) != 0 || uint64(data.length) != uint64(5+2*state.calls) {
		return 6
	}
	if state.consume && state.calls == 0 {
		C.sn_abi_v1_release(view)
	}
	state.calls++
	runtime.GC()
	return C.sn_test_native_bytes_push(view, 0)
}

//export sn_test_go_native_byte_arrays
func sn_test_go_native_byte_arrays() {
	before := C.sn_test_native_bytes_destroyed()
	native := C.sn_test_native_bytes_new()
	var view, copied *C.SnAbiValue
	check(C.sn_abi_v1_native_byte_array_borrow(native, &view) == 0)
	alias := C.sn_abi_v1_retain(view)
	var header *C.struct_SnArray
	check(C.sn_abi_v1_native_byte_array_data(alias, &header) == 0 && header == native)
	check(C.sn_abi_v1_native_byte_array_copy(view, &copied) == 0)
	state := &byteArrayState{}
	handle := cgo.NewHandle(state)
	check(C.sn_test_go_native_bytes_reenter(view, C.uintptr_t(handle)) == 0 && state.calls == 64)
	handle.Delete()
	var data C.SnAbiBytes
	check(C.sn_abi_v1_native_byte_array_bytes(alias, &data) == 0 && data.length == 132)
	C.sn_abi_v1_release(view)
	C.sn_abi_v1_release(alias)
	check(C.sn_test_native_bytes_destroyed() == before)
	C.sn_test_native_bytes_free(native)
	check(C.sn_abi_v1_native_byte_array_bytes(copied, &data) == 0)
	check(bytes.Equal(unsafe.Slice((*byte)(unsafe.Pointer(data.data)), int(data.length)), []byte{0, 127, 128, 255}))
	C.sn_abi_v1_release(copied)
	check(C.sn_test_native_bytes_destroyed() == before+136)
	check(C.sn_abi_v1_native_byte_array_adopt(C.sn_test_native_bytes_new(), &view) == 0)
	state = &byteArrayState{consume: true}
	handle = cgo.NewHandle(state)
	check(C.sn_test_go_native_bytes_reenter(view, C.uintptr_t(handle)) == 0 && state.calls == 64)
	handle.Delete()
	check(C.sn_test_native_bytes_destroyed() == before+268)
	owned := []byte{0, 255, 128, 0}
	check(C.sn_abi_v1_native_byte_array_copy_bytes((*C.uint8_t)(unsafe.Pointer(&owned[0])), C.uint64_t(len(owned)), &view) == 0)
	for i := range owned {
		owned[i] = 42
	}
	owned = nil
	runtime.GC()
	check(C.sn_abi_v1_native_byte_array_bytes(view, &data) == 0)
	check(bytes.Equal(unsafe.Slice((*byte)(unsafe.Pointer(data.data)), int(data.length)), []byte{0, 255, 128, 0}))
	C.sn_abi_v1_release(view)
}
