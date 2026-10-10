use std::ffi::{c_char, c_void, CString};
use std::sync::atomic::{AtomicUsize, Ordering};
#[repr(C)]
struct Value { _opaque: [u8; 0] }
#[repr(C)]
struct Bytes { data: *const u8, length: u64 }
#[link(name = "sn_runtime_min", kind = "static")]
extern "C" {
    fn sn_abi_v1_retain(value: *mut Value) -> *mut Value;
    fn sn_abi_v1_release(value: *mut Value);
    fn sn_abi_v1_native_string_array_borrow(array: *mut c_void, out: *mut *mut Value) -> u32;
    fn sn_abi_v1_native_string_array_adopt(array: *mut c_void, out: *mut *mut Value) -> u32;
    fn sn_abi_v1_native_string_array_data(value: *const Value, out: *mut *mut c_void) -> u32;
    fn sn_abi_v1_native_string_array_copy(value: *const Value, out: *mut *mut Value) -> u32;
    fn sn_abi_v1_string_bytes(value: *const Value, out: *mut Bytes) -> u32;
    fn sn_test_native_strings_new() -> *mut c_void;
    fn sn_test_native_strings_free(array: *mut c_void);
    fn sn_test_native_strings_destroyed() -> u64;
    fn sn_test_native_strings_length(value: *mut Value, out: *mut u64) -> u32;
    fn sn_test_native_strings_read(value: *mut Value, index: u64, out: *mut *mut Value) -> u32;
    fn sn_test_native_strings_push(value: *mut Value, text: *const c_char) -> u32;
    fn sn_test_native_strings_reenter(value: *mut Value,
        observe: extern "C" fn(*mut Value, usize) -> u32, context: usize) -> u32;
}
static CALLS: AtomicUsize = AtomicUsize::new(0);
extern "C" fn observe(view: *mut Value, context: usize) -> u32 {
    unsafe {
        let calls = CALLS.fetch_add(1, Ordering::SeqCst);
        let mut length = 0;
        if sn_test_native_strings_length(view, &mut length) != 0 || length != 5 + 2 * calls as u64 { return 6; }
        if context == 99 && calls == 0 { sn_abi_v1_release(view); }
        sn_test_native_strings_push(view, b"Rust callback\0".as_ptr().cast())
    }
}
fn main() {
    unsafe {
        let native = sn_test_native_strings_new();
        let mut view = std::ptr::null_mut();
        assert_eq!(sn_abi_v1_native_string_array_borrow(native, &mut view), 0);
        let alias = sn_abi_v1_retain(view);
        let mut pointer = std::ptr::null_mut();
        assert_eq!(sn_abi_v1_native_string_array_data(alias, &mut pointer), 0);
        assert_eq!(pointer, native);
        let mut copy = std::ptr::null_mut();
        assert_eq!(sn_abi_v1_native_string_array_copy(view, &mut copy), 0);
        let mut values = Vec::new();
        for index in 0..4 {
            let mut value = std::ptr::null_mut();
            assert_eq!(sn_test_native_strings_read(copy, index, &mut value), 0);
            values.push(value);
        }
        assert!(values[1].is_null());
        assert!(!values[2].is_null());
        assert_eq!(sn_test_native_strings_reenter(view, observe, 42), 0);
        assert_eq!(CALLS.load(Ordering::SeqCst), 64);
        let mut length = 0;
        assert_eq!(sn_test_native_strings_length(alias, &mut length), 0);
        assert_eq!(length, 132);
        assert_eq!(sn_test_native_strings_length(copy, &mut length), 0);
        assert_eq!(length, 4);
        sn_abi_v1_release(view);
        sn_abi_v1_release(alias);
        assert_eq!(sn_test_native_strings_destroyed(), 0);
        sn_test_native_strings_free(native);
        sn_abi_v1_release(copy);
        assert_eq!(sn_test_native_strings_destroyed(), 136);
        for (value, wanted) in values.into_iter().zip([Some(&b"one"[..]), None, Some(&b""[..]), Some(&[128, 255][..])]) {
            let mut bytes = Bytes { data: std::ptr::null(), length: 0 };
            assert_eq!(sn_abi_v1_string_bytes(value, &mut bytes), 0);
            if let Some(wanted) = wanted {
                assert!(!bytes.data.is_null());
                assert_eq!(std::slice::from_raw_parts(bytes.data, bytes.length as usize), wanted);
            } else { assert!(bytes.data.is_null()); }
            sn_abi_v1_release(value);
        }
        CALLS.store(0, Ordering::SeqCst);
        assert_eq!(sn_abi_v1_native_string_array_adopt(sn_test_native_strings_new(), &mut view), 0);
        assert_eq!(sn_test_native_strings_reenter(view, observe, 99), 0);
        assert_eq!(sn_test_native_strings_destroyed(), 268);
        let invalid = CString::new("unused").unwrap();
        assert_eq!(sn_test_native_strings_push(std::ptr::null_mut(), invalid.as_ptr()), 1);
    }
    println!("native string array views: pass");
}
