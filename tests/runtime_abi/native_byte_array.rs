use std::ffi::c_void;
use std::sync::atomic::{AtomicUsize, Ordering};
#[repr(C)]
struct Value { _opaque: [u8; 0] }
#[repr(C)]
struct Bytes { data: *const u8, length: u64 }
#[link(name = "sn_runtime_min", kind = "static")]
extern "C" {
    fn sn_abi_v1_retain(value: *mut Value) -> *mut Value;
    fn sn_abi_v1_release(value: *mut Value);
    fn sn_abi_v1_native_byte_array_borrow(array: *mut c_void, out: *mut *mut Value) -> u32;
    fn sn_abi_v1_native_byte_array_adopt(array: *mut c_void, out: *mut *mut Value) -> u32;
    fn sn_abi_v1_native_byte_array_data(value: *const Value, out: *mut *mut c_void) -> u32;
    fn sn_abi_v1_native_byte_array_bytes(value: *const Value, out: *mut Bytes) -> u32;
    fn sn_abi_v1_native_byte_array_copy(value: *const Value, out: *mut *mut Value) -> u32;
    fn sn_abi_v1_native_byte_array_copy_bytes(data: *const u8, length: u64, out: *mut *mut Value) -> u32;
    fn sn_test_native_bytes_new() -> *mut c_void;
    fn sn_test_native_bytes_free(array: *mut c_void);
    fn sn_test_native_bytes_destroyed() -> u64;
    fn sn_test_native_bytes_push(value: *mut Value, byte: u8) -> u32;
    fn sn_test_native_bytes_reenter(value: *mut Value,
        observe: extern "C" fn(*mut Value, usize) -> u32, context: usize) -> u32;
}
static CALLS: AtomicUsize = AtomicUsize::new(0);
extern "C" fn observe(view: *mut Value, context: usize) -> u32 {
    unsafe {
        let calls = CALLS.fetch_add(1, Ordering::SeqCst);
        let mut bytes = Bytes { data: std::ptr::null(), length: 0 };
        if sn_abi_v1_native_byte_array_bytes(view, &mut bytes) != 0 || bytes.length != 5 + 2 * calls as u64 { return 6; }
        if context == 99 && calls == 0 { sn_abi_v1_release(view); }
        sn_test_native_bytes_push(view, 0)
    }
}
fn main() {
    unsafe {
        let native = sn_test_native_bytes_new();
        let mut view = std::ptr::null_mut();
        assert_eq!(sn_abi_v1_native_byte_array_borrow(native, &mut view), 0);
        let alias = sn_abi_v1_retain(view);
        let mut pointer = std::ptr::null_mut();
        assert_eq!(sn_abi_v1_native_byte_array_data(alias, &mut pointer), 0);
        assert_eq!(pointer, native);
        let mut copy = std::ptr::null_mut();
        assert_eq!(sn_abi_v1_native_byte_array_copy(view, &mut copy), 0);
        assert_eq!(sn_test_native_bytes_reenter(view, observe, 42), 0);
        assert_eq!(CALLS.load(Ordering::SeqCst), 64);
        let mut bytes = Bytes { data: std::ptr::null(), length: 0 };
        assert_eq!(sn_abi_v1_native_byte_array_bytes(alias, &mut bytes), 0);
        assert_eq!(bytes.length, 132);
        sn_abi_v1_release(view); sn_abi_v1_release(alias);
        assert_eq!(sn_test_native_bytes_destroyed(), 0);
        sn_test_native_bytes_free(native);
        assert_eq!(sn_abi_v1_native_byte_array_bytes(copy, &mut bytes), 0);
        assert_eq!(std::slice::from_raw_parts(bytes.data, bytes.length as usize), &[0, 127, 128, 255]);
        sn_abi_v1_release(copy);
        assert_eq!(sn_test_native_bytes_destroyed(), 136);
        CALLS.store(0, Ordering::SeqCst);
        assert_eq!(sn_abi_v1_native_byte_array_adopt(sn_test_native_bytes_new(), &mut view), 0);
        assert_eq!(sn_test_native_bytes_reenter(view, observe, 99), 0);
        assert_eq!(sn_test_native_bytes_destroyed(), 268);
        let mut owned = vec![0, 255, 128, 0];
        assert_eq!(sn_abi_v1_native_byte_array_copy_bytes(owned.as_ptr(), owned.len() as u64, &mut view), 0);
        owned.fill(42); drop(owned);
        assert_eq!(sn_abi_v1_native_byte_array_bytes(view, &mut bytes), 0);
        assert_eq!(std::slice::from_raw_parts(bytes.data, bytes.length as usize), &[0, 255, 128, 0]);
        sn_abi_v1_release(view);
        assert_eq!(sn_abi_v1_native_byte_array_copy_bytes([0u8].as_ptr(), 0, &mut view), 0);
        assert!(!view.is_null()); sn_abi_v1_release(view);
        assert_eq!(sn_abi_v1_native_byte_array_copy_bytes(std::ptr::null(), 0, &mut view), 0);
        assert!(view.is_null());
    }
    println!("native byte array views: pass");
}
