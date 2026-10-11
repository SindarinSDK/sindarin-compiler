#!/usr/bin/env python3
"""Go native backing preserves canonical C records under cgocheck2 and forced GC."""
import os
import unittest
import rust_records as fixtures


class GoRecordTests(fixtures.RustRecordTests):
    backing_language = 'GO'
    panic_stderr = False

    def setUp(self):
        super().setUp()
        self.manifest = self.manifest.replace(
            '      language: RS\n      entry: src/backing.rs\n      sources: [src/backing.rs]\n',
            '      language: GO\n      module: src/go\n      sources: [src/go/backing.go]\n')
        for old, new in (('make_rust','Make'), ('borrow_rust','Borrow'), ('checked_rust','Checked')):
            self.manifest = self.manifest.replace('function: ' + old + '\n', 'function: ' + new + '\n')
        self.write('.sn/methods/src/go/go.mod', 'module sindarin.test/recordbacking\n\ngo 1.26.0\n')
        self.write('.sn/methods/src/go/backing.go', BACKING)
        self.write('.sn/methods/sn.yaml', self.manifest)


BACKING = r'''package recordbacking
/*
#include "../counter.h"
*/
import "C"
import "unsafe"
import "runtime"

func Make(value int64) (unsafe.Pointer, uint32) {
    runtime.GC()
    if value < 0 { return nil, 0 }
    p := C.counter_create()
    p.__sn__amount = C.longlong(value)
    return unsafe.Pointer(p), 0
}
func Borrow(a unsafe.Pointer, b unsafe.Pointer) (unsafe.Pointer, uint32) {
    runtime.GC()
    if a != b { panic("record identities differ") }
    if a != nil { (*C.CanonicalCounter)(a).__sn__amount++ }
    return a, 0
}
func Checked(a unsafe.Pointer, b unsafe.Pointer, mode int64) (unsafe.Pointer, uint32) {
    runtime.GC()
    if mode == 1 { return nil, 5 }
    if mode == 2 { panic("record foreign panic") }
    if mode == 3 { C.counter_visit(); runtime.GC() }
    return Borrow(a, b)
}
'''

if __name__ == '__main__':
    os.environ['GOEXPERIMENT'] = 'cgocheck2'
    unittest.main(verbosity=2)
