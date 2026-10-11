package backing

/* #include "../ops.h" */
import "C"
import (
	"runtime"
	"unsafe"
)

func Provide(a, b unsafe.Pointer, mode int64) (unsafe.Pointer, uint32) {
	if mode == 2 {
		panic("typed provider panic")
	}
	if mode == 3 {
		return nil, 5
	}
	value := C.sn_typed_copy(a, b, C.int64_t(mode))
	observe := func() { runtime.GC(); C.sn_typed_observe(b) }
	observe()
	if mode == 4 {
		C.sn_typed_free(value)
		return nil, 5
	}
	return value, 0
}
func ProvideInts(a, b unsafe.Pointer) (unsafe.Pointer, uint32) {
	value := C.sn_typed_ints(a, b)
	runtime.GC()
	return value, 0
}
