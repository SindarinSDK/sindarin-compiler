use std::ffi::{c_char, c_void};
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
    fn sn_abi_v1_value_array_push(array: *mut Value, value: *mut Value) -> u32;
    fn sn_abi_v1_value_array_get(array: *const Value, index: u64, out: *mut *mut Value) -> u32;
    fn sn_abi_v1_value_array_length(array: *const Value, out: *mut u64) -> u32;
    fn sn_abi_v1_value_array_assign(destination: *mut Value, source: *const Value) -> u32;
    fn sn_abi_v1_resource_new(
        data: *mut c_void,
        destroy: extern "C" fn(*mut c_void, usize),
        context: usize,
        out: *mut *mut Value,
    ) -> u32;
}
static DESTROYED: AtomicUsize = AtomicUsize::new(0);
extern "C" fn destroy(_data: *mut c_void, _context: usize) {
    DESTROYED.fetch_add(1, Ordering::SeqCst);
}
fn main() {
    unsafe {
        let mut info = Info::default();
        for (version, mask) in [(0x10000, 7), (0x10001, 31), (0x10002, 63)] {
            assert_eq!(sn_abi_v1_query(version, 0, &mut info, 32), 0);
            assert_eq!((info.version, info.capabilities), (version, mask));
            assert_eq!(sn_abi_v1_query(version, 64, &mut info, 32), 3);
            assert_eq!((info.version, info.capabilities), (version, mask));
        }
        assert_eq!(sn_abi_v1_query(0x10003, 64, &mut info, 32), 0);
        assert_eq!((info.version, info.capabilities), (0x10003, 127));
        let mut source = std::ptr::null_mut();
        let mut destination = std::ptr::null_mut();
        let mut resource = std::ptr::null_mut();
        let mut text = std::ptr::null_mut();
        assert_eq!(sn_abi_v1_value_array_new(&mut source), 0);
        assert_eq!(sn_abi_v1_value_array_new(&mut destination), 0);
        assert_eq!(
            sn_abi_v1_resource_new(std::ptr::null_mut(), destroy, 0, &mut resource),
            0
        );
        assert_eq!(sn_abi_v1_value_array_push(source, resource), 0);
        assert_eq!(sn_abi_v1_value_array_push(destination, resource), 0);
        sn_abi_v1_release(resource);
        assert_eq!(
            sn_abi_v1_string_copy(b"\x80\xff\0".as_ptr().cast(), &mut text),
            0
        );
        assert_eq!(sn_abi_v1_value_array_push(source, text), 0);
        assert_eq!(sn_abi_v1_value_array_push(source, std::ptr::null_mut()), 0);
        let alias = sn_abi_v1_retain(destination);
        assert_eq!(sn_abi_v1_value_array_assign(destination, text), 4);
        let mut length = 99;
        assert_eq!(sn_abi_v1_value_array_length(alias, &mut length), 0);
        assert_eq!(length, 1);
        assert_eq!(DESTROYED.load(Ordering::SeqCst), 0);
        assert_eq!(sn_abi_v1_value_array_assign(destination, source), 0);
        assert_eq!(sn_abi_v1_value_array_assign(destination, alias), 0);
        sn_abi_v1_release(source);
        sn_abi_v1_release(destination);
        assert_eq!(sn_abi_v1_value_array_length(alias, &mut length), 0);
        assert_eq!(length, 3);
        assert_eq!(sn_abi_v1_value_array_get(alias, 0, &mut resource), 0);
        assert_eq!(sn_abi_v1_value_array_assign(alias, std::ptr::null()), 0);
        assert_eq!(sn_abi_v1_value_array_length(alias, &mut length), 0);
        assert_eq!(length, 0);
        assert_eq!(DESTROYED.load(Ordering::SeqCst), 0);
        assert_eq!(sn_abi_v1_value_array_assign(std::ptr::null_mut(), alias), 1);
        sn_abi_v1_release(alias);
        sn_abi_v1_release(resource);
        assert_eq!(DESTROYED.load(Ordering::SeqCst), 1);
        let mut bytes = Bytes {
            data: std::ptr::null(),
            length: 0,
        };
        assert_eq!(sn_abi_v1_string_bytes(text, &mut bytes), 0);
        assert_eq!(
            std::slice::from_raw_parts(bytes.data, bytes.length as usize),
            b"\x80\xff"
        );
        sn_abi_v1_release(text);
        println!("managed array replacement: pass");
    }
}
