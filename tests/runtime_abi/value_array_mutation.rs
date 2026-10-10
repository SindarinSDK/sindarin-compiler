use std::ffi::{c_char, c_void};
use std::ptr::null_mut;
use std::sync::atomic::{AtomicUsize, Ordering};

#[repr(C)]
struct Value {
    _opaque: [u8; 0],
}
#[repr(C)]
#[derive(Default)]
struct Info {
    version: u32,
    pointer_bits: u32,
    capabilities: u64,
    int_bits: u32,
    char_bits: u32,
    float_bits: u32,
    double_bits: u32,
}
#[repr(C)]
struct Bytes {
    data: *const u8,
    length: u64,
}
#[link(name = "sn_runtime_min", kind = "static")]
extern "C" {
    fn sn_abi_v1_query(version: u32, capabilities: u64, info: *mut Info, size: u32) -> u32;
    fn sn_abi_v1_retain(value: *mut Value) -> *mut Value;
    fn sn_abi_v1_release(value: *mut Value);
    fn sn_abi_v1_string_copy(text: *const c_char, out: *mut *mut Value) -> u32;
    fn sn_abi_v1_string_bytes(value: *const Value, out: *mut Bytes) -> u32;
    fn sn_abi_v1_value_array_new(out: *mut *mut Value) -> u32;
    fn sn_abi_v1_value_array_copy(array: *const Value, out: *mut *mut Value) -> u32;
    fn sn_abi_v1_value_array_get(array: *const Value, index: u64, out: *mut *mut Value) -> u32;
    fn sn_abi_v1_value_array_length(array: *const Value, out: *mut u64) -> u32;
    fn sn_abi_v1_value_array_insert(array: *mut Value, index: u64, value: *mut Value) -> u32;
    fn sn_abi_v1_value_array_take(array: *mut Value, index: u64, out: *mut *mut Value) -> u32;
    fn sn_abi_v1_value_array_pop(array: *mut Value, out: *mut *mut Value) -> u32;
    fn sn_abi_v1_value_array_remove(array: *mut Value, index: u64) -> u32;
    fn sn_abi_v1_value_array_clear(array: *mut Value) -> u32;
    fn sn_abi_v1_value_array_reverse(array: *mut Value) -> u32;
    fn sn_abi_v1_resource_new(
        data: *mut c_void,
        destroy: extern "C" fn(*mut c_void, usize),
        context: usize,
        out: *mut *mut Value,
    ) -> u32;
}
static DESTROYED: AtomicUsize = AtomicUsize::new(0);
extern "C" fn destroy(data: *mut c_void, context: usize) {
    assert!(data.is_null());
    assert_eq!(context, 7);
    DESTROYED.fetch_add(1, Ordering::SeqCst);
}
struct State {
    array: *mut Value,
    marker: *mut Value,
    calls: usize,
}
extern "C" fn reenter(data: *mut c_void, context: usize) {
    assert!(data.is_null());
    unsafe {
        let state = &mut *(context as *mut State);
        let mut length = 99;
        assert_eq!(sn_abi_v1_value_array_length(state.array, &mut length), 0);
        assert_eq!(length, 0);
        // Consume the caller credit before relocation and nested mutation.
        sn_abi_v1_release(state.array);
        for _ in 0..64 {
            assert_eq!(sn_abi_v1_value_array_insert(state.array, 0, null_mut()), 0);
        }
        assert_eq!(sn_abi_v1_value_array_reverse(state.array), 0);
        assert_eq!(sn_abi_v1_value_array_clear(state.array), 0);
        assert_eq!(
            sn_abi_v1_value_array_insert(state.array, 0, state.marker),
            0
        );
        state.calls += 1;
    }
}
fn main() {
    unsafe {
        let mut info = Info::default();
        for (version, mask) in [(0x10000, 7), (0x10001, 31), (0x10002, 63), (0x10003, 127)] {
            assert_eq!(sn_abi_v1_query(version, 0, &mut info, 32), 0);
            assert_eq!((info.version, info.capabilities), (version, mask));
            assert_eq!(sn_abi_v1_query(version, 128, &mut info, 32), 3);
            assert_eq!((info.version, info.capabilities), (version, mask));
        }
        assert_eq!(sn_abi_v1_query(0x10004, 128, &mut info, 32), 0);
        assert_eq!((info.version, info.capabilities), (0x10004, 255));
        let (mut array, mut text, mut resource) = (null_mut(), null_mut(), null_mut());
        assert_eq!(sn_abi_v1_value_array_new(&mut array), 0);
        assert_eq!(
            sn_abi_v1_string_copy(b"\x80\xff\0".as_ptr().cast(), &mut text),
            0
        );
        let mut out = text;
        assert_eq!(sn_abi_v1_value_array_pop(array, &mut out), 5);
        assert_eq!(out, text);
        assert_eq!(sn_abi_v1_value_array_take(text, 0, &mut out), 4);
        assert_eq!(out, text);
        assert_eq!(sn_abi_v1_value_array_insert(array, u64::MAX, text), 5);
        assert_eq!(sn_abi_v1_value_array_insert(array, 0, text), 0);
        assert_eq!(sn_abi_v1_value_array_insert(array, 0, null_mut()), 0);
        let alias = sn_abi_v1_retain(array);
        assert_eq!(sn_abi_v1_value_array_reverse(alias), 0);
        assert_eq!(sn_abi_v1_value_array_pop(array, &mut out), 0);
        assert!(out.is_null());
        assert_eq!(sn_abi_v1_value_array_get(alias, 0, &mut out), 0);
        assert_eq!(out, text);
        sn_abi_v1_release(out);
        assert_eq!(sn_abi_v1_value_array_take(alias, 0, &mut out), 0);
        assert_eq!(out, text);
        sn_abi_v1_release(array);
        sn_abi_v1_release(alias);
        let mut view = Bytes {
            data: std::ptr::null(),
            length: 0,
        };
        assert_eq!(sn_abi_v1_string_bytes(out, &mut view), 0);
        assert_eq!(
            std::slice::from_raw_parts(view.data, view.length as usize),
            b"\x80\xff"
        );
        sn_abi_v1_release(out);

        assert_eq!(sn_abi_v1_value_array_new(&mut array), 0);
        assert_eq!(
            sn_abi_v1_resource_new(null_mut(), destroy, 7, &mut resource),
            0
        );
        assert_eq!(sn_abi_v1_value_array_insert(array, 0, resource), 0);
        sn_abi_v1_release(resource);
        let mut copy = null_mut();
        assert_eq!(sn_abi_v1_value_array_copy(array, &mut copy), 0);
        assert_eq!(sn_abi_v1_value_array_remove(array, 0), 0);
        assert_eq!(DESTROYED.load(Ordering::SeqCst), 0);
        assert_eq!(sn_abi_v1_value_array_pop(copy, &mut out), 0);
        assert_eq!(out, resource);
        sn_abi_v1_release(copy);
        sn_abi_v1_release(array);
        assert_eq!(DESTROYED.load(Ordering::SeqCst), 0);
        sn_abi_v1_release(out);
        assert_eq!(DESTROYED.load(Ordering::SeqCst), 1);

        for clear in [false, true] {
            assert_eq!(sn_abi_v1_value_array_new(&mut array), 0);
            let mut state = State {
                array,
                marker: text,
                calls: 0,
            };
            assert_eq!(
                sn_abi_v1_resource_new(
                    null_mut(),
                    reenter,
                    &mut state as *mut State as usize,
                    &mut resource
                ),
                0
            );
            assert_eq!(sn_abi_v1_value_array_insert(array, 0, resource), 0);
            sn_abi_v1_release(resource);
            assert_eq!(
                if clear {
                    sn_abi_v1_value_array_clear(array)
                } else {
                    sn_abi_v1_value_array_remove(array, 0)
                },
                0
            );
            assert_eq!(state.calls, 1);
        }
        sn_abi_v1_release(text);
        println!("managed array mutation: pass");
    }
}
