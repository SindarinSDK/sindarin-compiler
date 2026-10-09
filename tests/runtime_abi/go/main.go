package main

/*
#include "sn_abi.h"
#include <stdlib.h>
SnAbiStatus sn_test_go_resource(uintptr_t context, SnAbiValue **out);
SnAbiStatus sn_test_go_typed_resource(uintptr_t context, SnAbiValue **out);
*/
import "C"
import (
	"fmt"
	"runtime"
	"runtime/cgo"
	"sync/atomic"
	"unsafe"
)

var nativeDestroyed atomic.Uint64

type resourceState struct {
	destroyed int
	value     int
}

//export sn_test_go_destroy
func sn_test_go_destroy(data unsafe.Pointer, context C.uintptr_t) {
	// A native-sized registry token is retained in C, never a Go object pointer.
	handle := cgo.Handle(context)
	state := handle.Value().(*resourceState)
	state.destroyed++
	nativeDestroyed.Add(1)
	state.value = 42
	handle.Delete()
	var nested *C.SnAbiValue
	text := C.CString("reentrant")
	status := C.sn_abi_v1_string_copy(text, &nested)
	C.free(unsafe.Pointer(text))
	if status == C.SN_ABI_OK {
		C.sn_abi_v1_release(nested)
	}
}

//export sn_test_go_text
func sn_test_go_text(out **C.SnAbiValue) C.SnAbiStatus {
	text := C.CString("native Go backing")
	status := C.sn_abi_v1_string_copy(text, out)
	C.free(unsafe.Pointer(text))
	runtime.GC()
	return status
}

//export sn_test_go_owned_resource
func sn_test_go_owned_resource(out **C.SnAbiValue) C.SnAbiStatus {
	handle := cgo.NewHandle(&resourceState{})
	status := C.sn_test_go_resource(C.uintptr_t(handle), out)
	if status != C.SN_ABI_OK {
		handle.Delete()
	}
	return status
}

//export sn_test_go_cleanup_count
func sn_test_go_cleanup_count() C.uint64_t {
	return C.uint64_t(nativeDestroyed.Load())
}

func check(ok bool) {
	if !ok {
		panic("runtime ABI contract failed")
	}
}
func main() {
	var info C.SnAbiInfo
	check(C.sn_abi_v1_query(C.SN_ABI_V1_VERSION, 7, &info, C.sizeof_SnAbiInfo) == C.SN_ABI_OK)
	check(info.int_bits == 64 && info.char_bits == 8 && info.float_bits == 32 && info.double_bits == 64)
	check(info.pointer_bits == C.uint32_t(unsafe.Sizeof(uintptr(0))*8))
	check(C.sn_abi_v1_query(0, 0, &info, C.sizeof_SnAbiInfo) == C.SN_ABI_VERSION_MISMATCH)
	var value *C.SnAbiValue
	text := C.CString("shared")
	check(C.sn_abi_v1_string_copy(text, &value) == C.SN_ABI_OK)
	C.free(unsafe.Pointer(text))
	alias := C.sn_abi_v1_retain(value)
	C.sn_abi_v1_release(value)
	runtime.GC()
	var view C.SnAbiBytes
	check(C.sn_abi_v1_bytes(alias, &view) == C.SN_ABI_OK)
	check(string(C.GoBytes(unsafe.Pointer(view.data), C.int(view.length))) == "shared")
	var joined *C.SnAbiValue
	check(C.sn_abi_v1_string_concat(alias, alias, &joined) == C.SN_ABI_OK)
	C.sn_abi_v1_release(alias)
	check(C.sn_abi_v1_bytes(joined, &view) == C.SN_ABI_OK)
	check(string(C.GoBytes(unsafe.Pointer(view.data), C.int(view.length))) == "sharedshared")
	C.sn_abi_v1_release(joined)
	binary := []byte{0, 128, 255, 0}
	input := C.CBytes(binary)
	check(C.sn_abi_v1_buffer_copy((*C.uint8_t)(input), C.uint64_t(len(binary)), &value) == C.SN_ABI_OK)
	C.free(input)
	runtime.GC()
	check(C.sn_abi_v1_bytes(value, &view) == C.SN_ABI_OK)
	copied := C.GoBytes(unsafe.Pointer(view.data), C.int(view.length))
	check(string(copied) == string(binary))
	C.sn_abi_v1_release(value)
	check(C.sn_abi_v1_string_copy(nil, &value) == C.SN_ABI_OK && value == nil)
	check(C.sn_abi_v1_bytes(value, &view) == C.SN_ABI_OK && view.data == nil)
	check(C.sn_abi_v1_array_new(8, &value) == C.SN_ABI_OK)
	for _, number := range []C.int64_t{-9223372036854775808, 7, 9223372036854775807} {
		// C copies scalar bytes synchronously and does not retain this Go pointer.
		check(C.sn_abi_v1_array_push(value, unsafe.Pointer(&number), 8) == C.SN_ABI_OK)
	}
	var copy *C.SnAbiValue
	check(C.sn_abi_v1_array_copy(value, &copy) == C.SN_ABI_OK)
	alias = C.sn_abi_v1_retain(value)
	C.sn_abi_v1_release(value)
	number := C.int64_t(42)
	check(C.sn_abi_v1_array_set(alias, 1, unsafe.Pointer(&number), 8) == C.SN_ABI_OK)
	check(C.sn_abi_v1_array_get(alias, 1, unsafe.Pointer(&number), 8) == C.SN_ABI_OK && number == 42)
	check(C.sn_abi_v1_array_get(copy, 1, unsafe.Pointer(&number), 8) == C.SN_ABI_OK && number == 7)
	check(C.sn_abi_v1_array_get(copy, 99, unsafe.Pointer(&number), 8) == C.SN_ABI_OUT_OF_RANGE && number == 7)
	C.sn_abi_v1_release(alias)
	C.sn_abi_v1_release(copy)
	state := &resourceState{}
	handle := cgo.NewHandle(state)
	check(C.sn_test_go_resource(C.uintptr_t(handle), &value) == C.SN_ABI_OK)
	alias = C.sn_abi_v1_retain(value)
	C.sn_abi_v1_release(value)
	runtime.GC()
	check(state.destroyed == 0)
	C.sn_abi_v1_release(alias)
	check(state.destroyed == 1 && state.value == 42)
	state = &resourceState{}
	handle = cgo.NewHandle(state)
	check(C.sn_test_go_typed_resource(C.uintptr_t(handle), &value) == C.SN_ABI_OK)
	var identity *C.char
	check(C.sn_abi_v1_resource_type(value, &identity) == C.SN_ABI_OK)
	check(C.GoString(identity) == "pkg.GoResource@1")
	wrong := C.CString("pkg.OtherResource@1")
	var data unsafe.Pointer
	check(C.sn_abi_v1_resource_data_typed(value, wrong, &data) == C.SN_ABI_WRONG_KIND)
	C.free(unsafe.Pointer(wrong))
	alias = C.sn_abi_v1_retain(value)
	C.sn_abi_v1_release(value)
	runtime.GC()
	check(state.destroyed == 0)
	check(C.sn_abi_v1_resource_data_typed(alias, identity, &data) == C.SN_ABI_OK)
	C.sn_abi_v1_release(alias)
	check(state.destroyed == 1 && state.value == 42)
	check(C.sn_abi_v1_query(C.SN_ABI_V1_1_VERSION, C.SN_ABI_CAP_VALUE_ARRAYS, &info, C.sizeof_SnAbiInfo) == C.SN_ABI_OK)
	check(info.abi_version == C.SN_ABI_V1_1_VERSION && info.capabilities == 31)
	var values, copiedValues *C.SnAbiValue
	check(C.sn_abi_v1_value_array_new(&values) == C.SN_ABI_OK)
	text = C.CString("managed")
	check(C.sn_abi_v1_string_copy(text, &value) == C.SN_ABI_OK)
	C.free(unsafe.Pointer(text))
	check(C.sn_abi_v1_value_array_push(values, value) == C.SN_ABI_OK)
	C.sn_abi_v1_release(value)
	check(C.sn_abi_v1_value_array_copy(values, &copiedValues) == C.SN_ABI_OK)
	check(C.sn_abi_v1_value_array_set(values, 0, nil) == C.SN_ABI_OK)
	C.sn_abi_v1_release(values)
	runtime.GC()
	check(C.sn_abi_v1_value_array_get(copiedValues, 0, &value) == C.SN_ABI_OK)
	C.sn_abi_v1_release(copiedValues)
	check(C.sn_abi_v1_bytes(value, &view) == C.SN_ABI_OK)
	check(string(C.GoBytes(unsafe.Pointer(view.data), C.int(view.length))) == "managed")
	C.sn_abi_v1_release(value)
	fmt.Println("shared runtime ABI: pass")
}
