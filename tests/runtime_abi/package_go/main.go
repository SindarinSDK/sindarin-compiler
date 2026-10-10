package main
/*
#include "sn_abi.h"
SnAbiStatus sn_go_package_new(uintptr_t context, SnAbiPackage **out);
*/
import "C"
import (
    "fmt"
    "runtime"
    "runtime/cgo"
)
type state struct { initialized, cleaned int }
func require(ok bool) { if !ok { panic("package lifecycle invariant") } }
//export sn_go_package_init
func sn_go_package_init(packageValue *C.SnAbiPackage, context C.uintptr_t) C.uint32_t {
    s:=cgo.Handle(context).Value().(*state); s.initialized++; require(s.initialized==1)
    var call *C.SnAbiPackageCall
    require(C.sn_abi_v1_package_begin(packageValue,&call)==0)
    require(C.sn_abi_v1_package_shutdown(packageValue)==7)
    C.sn_abi_v1_package_end(call)
    return 0
}
//export sn_go_package_cleanup
func sn_go_package_cleanup(packageValue *C.SnAbiPackage, context C.uintptr_t) {
    h:=cgo.Handle(context); s:=h.Value().(*state);s.cleaned++;require(s.cleaned==1)
    require(C.sn_abi_v1_package_shutdown(packageValue)==0);h.Delete()
}
func main() {
    runtime.LockOSThread();defer runtime.UnlockOSThread()
    s:=&state{};h:=cgo.NewHandle(s)
    var p *C.SnAbiPackage;var call *C.SnAbiPackageCall
    require(C.sn_go_package_new(C.uintptr_t(h),&p)==0)
    require(C.sn_abi_v1_package_begin(p,&call)==0)
    C.sn_abi_v1_package_release(p);require(s.cleaned==0)
    C.sn_abi_v1_package_end(call);require(s.cleaned==1)
    fmt.Println("package lifecycle: pass")
}
