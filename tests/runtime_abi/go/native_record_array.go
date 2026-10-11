package main

/*
#include "native_record_array_bridge.h"
#include <stdlib.h>
SnAbiStatus sn_test_go_record_array_reenter(SnAbiValue *, uintptr_t);
*/
import "C"
import (
    "runtime"
    "runtime/cgo"
    "unsafe"
)

type recordArrayState struct { calls int; consume bool }

//export sn_test_go_record_array_observe
func sn_test_go_record_array_observe(view *C.SnAbiValue, context C.uintptr_t) C.SnAbiStatus {
    state := cgo.Handle(context).Value().(*recordArrayState)
    check(C.sn_test_record_array_value(view) == C.int64_t(state.calls+8))
    if state.consume && state.calls == 0 { C.sn_abi_v1_release(view) }
    state.calls++
    runtime.GC()
    return 0
}

//export sn_test_go_record_arrays
func sn_test_go_record_arrays() {
    before := C.sn_test_record_array_destroyed()
    identity := C.CString("pkg.Record@1")
    defer C.free(unsafe.Pointer(identity))
    original := C.sn_test_record_array_new()
    var view, copied *C.SnAbiValue
    check(C.sn_abi_v1_native_record_array_borrow(original, 1, identity, &view) == 0)
    var header *C.struct_SnArray
    check(C.sn_abi_v1_native_record_array_data(view, 1, identity, &header) == 0 && header == original)
    check(C.sn_abi_v1_native_record_array_copy(view, &copied) == 0)
    state := &recordArrayState{}
    handle := cgo.NewHandle(state)
    check(C.sn_test_go_record_array_reenter(view, C.uintptr_t(handle)) == 0 && state.calls == 64)
    handle.Delete()
    check(C.sn_test_record_array_value(copied) == 71)
    C.sn_abi_v1_release(view)
    C.sn_test_record_array_free(original)
    check(C.sn_test_record_array_destroyed() == before)
    check(C.sn_abi_v1_native_record_array_take(&copied, 1, identity, &header) == 0 && copied == nil)
    C.sn_test_record_array_free(header)
    check(C.sn_test_record_array_destroyed() == before+1)
    check(C.sn_abi_v1_native_record_array_adopt(C.sn_test_record_array_new(), 1, identity, &view) == 0)
    state = &recordArrayState{consume:true}
    handle = cgo.NewHandle(state)
    check(C.sn_test_go_record_array_reenter(view, C.uintptr_t(handle)) == 0 && state.calls == 64)
    handle.Delete()
    check(C.sn_test_record_array_destroyed() == before+2)
}
