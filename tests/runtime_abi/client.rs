use std::ffi::{c_char, c_void, CString};
use std::sync::atomic::{AtomicUsize, Ordering};
#[repr(C)]
struct Value {
    _private: [u8; 0],
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
    fn sn_abi_v1_query(version: u32, capabilities: u64, out: *mut Info, size: u32) -> u32;
    fn sn_abi_v1_retain(value: *mut Value) -> *mut Value;
    fn sn_abi_v1_release(value: *mut Value);
    fn sn_abi_v1_string_copy(text: *const c_char, out: *mut *mut Value) -> u32;
    fn sn_abi_v1_string_concat(
        left: *const Value,
        right: *const Value,
        out: *mut *mut Value,
    ) -> u32;
    fn sn_abi_v1_buffer_copy(data: *const u8, length: u64, out: *mut *mut Value) -> u32;
    fn sn_abi_v1_bytes(value: *const Value, out: *mut Bytes) -> u32;
    fn sn_abi_v1_array_new(size: u64, out: *mut *mut Value) -> u32;
    fn sn_abi_v1_array_push(value: *mut Value, data: *const c_void, size: u64) -> u32;
    fn sn_abi_v1_array_get(value: *const Value, index: u64, data: *mut c_void, size: u64) -> u32;
    fn sn_abi_v1_array_set(value: *mut Value, index: u64, data: *const c_void, size: u64) -> u32;
    fn sn_abi_v1_array_copy(value: *const Value, out: *mut *mut Value) -> u32;
    fn sn_abi_v1_resource_new(
        data: *mut c_void,
        destroy: extern "C" fn(*mut c_void, usize),
        context: usize,
        out: *mut *mut Value,
    ) -> u32;
    fn sn_abi_v1_resource_data(value: *const Value, out: *mut *mut c_void) -> u32;
    fn sn_abi_v1_resource_new_typed(
        identity: *const c_char,
        data: *mut c_void,
        destroy: extern "C" fn(*mut c_void, usize),
        context: usize,
        out: *mut *mut Value,
    ) -> u32;
    fn sn_abi_v1_resource_data_typed(
        value: *const Value,
        identity: *const c_char,
        out: *mut *mut c_void,
    ) -> u32;
    fn sn_abi_v1_resource_type(value: *const Value, out: *mut *const c_char) -> u32;
}
#[cfg(go_bridge)]
#[link(name = "go_runtime_abi", kind = "static")]
extern "C" {
    fn sn_test_go_text(out: *mut *mut Value) -> u32;
    fn sn_test_go_owned_resource(out: *mut *mut Value) -> u32;
    fn sn_test_go_cleanup_count() -> u64;
}

static DESTROYED: AtomicUsize = AtomicUsize::new(0);
extern "C" fn destroy(data: *mut c_void, context: usize) {
    // Real generated callbacks must contain panics. This callback has no panic path.
    unsafe {
        drop(Box::from_raw(data as *mut i64));
    }
    DESTROYED.fetch_add(context, Ordering::SeqCst);
    unsafe {
        let mut nested = std::ptr::null_mut();
        let text = b"reentrant\0";
        if sn_abi_v1_string_copy(text.as_ptr() as *const c_char, &mut nested) == 0 {
            sn_abi_v1_release(nested);
        }
    }
}
fn main() {
    unsafe {
        let mut info = Info::default();
        assert_eq!(std::mem::size_of::<Info>(), 32);
        assert_eq!(sn_abi_v1_query(0x10000, 7, &mut info, 32), 0);
        assert_eq!(
            (
                info.version,
                info.capabilities,
                info.int_bits,
                info.char_bits,
                info.float_bits,
                info.double_bits
            ),
            (0x10000, 7, 64, 8, 32, 64)
        );
        assert_eq!(info.pointer_bits, usize::BITS);
        assert_eq!(sn_abi_v1_query(0, 0, &mut info, 32), 2);
        let mut value = std::ptr::null_mut();
        let input = CString::new("shared").unwrap();
        assert_eq!(sn_abi_v1_string_copy(input.as_ptr(), &mut value), 0);
        drop(input);
        let alias = sn_abi_v1_retain(value);
        sn_abi_v1_release(value);
        let mut view = Bytes {
            data: std::ptr::null(),
            length: 0,
        };
        assert_eq!(sn_abi_v1_bytes(alias, &mut view), 0);
        assert_eq!(
            std::slice::from_raw_parts(view.data, view.length as usize),
            b"shared"
        );
        let mut concat = std::ptr::null_mut();
        assert_eq!(sn_abi_v1_string_concat(alias, alias, &mut concat), 0);
        sn_abi_v1_release(alias);
        assert_eq!(sn_abi_v1_bytes(concat, &mut view), 0);
        assert_eq!(
            std::slice::from_raw_parts(view.data, view.length as usize),
            b"sharedshared"
        );
        sn_abi_v1_release(concat);
        let input = [0u8, 128, 255, 0];
        assert_eq!(
            sn_abi_v1_buffer_copy(input.as_ptr(), input.len() as u64, &mut value),
            0
        );
        assert_eq!(sn_abi_v1_bytes(value, &mut view), 0);
        assert_eq!(
            std::slice::from_raw_parts(view.data, view.length as usize),
            input
        );
        sn_abi_v1_release(value);
        assert_eq!(sn_abi_v1_string_copy(std::ptr::null(), &mut value), 0);
        assert!(value.is_null());
        assert_eq!(sn_abi_v1_bytes(value, &mut view), 0);
        assert!(view.data.is_null());
        assert_eq!(sn_abi_v1_array_new(8, &mut value), 0);
        for number in [i64::MIN, 7, i64::MAX] {
            assert_eq!(
                sn_abi_v1_array_push(value, &number as *const _ as *const c_void, 8),
                0
            );
        }
        let mut copy = std::ptr::null_mut();
        assert_eq!(sn_abi_v1_array_copy(value, &mut copy), 0);
        let alias = sn_abi_v1_retain(value);
        sn_abi_v1_release(value);
        let mut number: i64 = 42;
        assert_eq!(
            sn_abi_v1_array_set(alias, 1, &number as *const _ as *const c_void, 8),
            0
        );
        assert_eq!(
            sn_abi_v1_array_get(alias, 1, &mut number as *mut _ as *mut c_void, 8),
            0
        );
        assert_eq!(number, 42);
        assert_eq!(
            sn_abi_v1_array_get(copy, 1, &mut number as *mut _ as *mut c_void, 8),
            0
        );
        assert_eq!(number, 7);
        assert_eq!(
            sn_abi_v1_array_get(copy, 99, &mut number as *mut _ as *mut c_void, 8),
            5
        );
        assert_eq!(number, 7);
        assert_eq!(
            sn_abi_v1_array_get(copy, 0, &mut number as *mut _ as *mut c_void, 8),
            0
        );
        assert_eq!(number, i64::MIN);
        sn_abi_v1_release(alias);
        sn_abi_v1_release(copy);
        let data = Box::into_raw(Box::new(42i64)) as *mut c_void;
        assert_eq!(sn_abi_v1_resource_new(data, destroy, 1, &mut value), 0);
        let alias = sn_abi_v1_retain(value);
        sn_abi_v1_release(value);
        assert_eq!(DESTROYED.load(Ordering::SeqCst), 0);
        let mut borrowed = std::ptr::null_mut();
        assert_eq!(sn_abi_v1_resource_data(alias, &mut borrowed), 0);
        assert_eq!(borrowed, data);
        sn_abi_v1_release(alias);
        assert_eq!(DESTROYED.load(Ordering::SeqCst), 1);
        let identity = CString::new("pkg.RustResource@1").unwrap();
        let data = Box::into_raw(Box::new(42i64)).cast::<c_void>();
        let mut typed = std::ptr::null_mut();
        assert_eq!(
            sn_abi_v1_resource_new_typed(identity.as_ptr(), data, destroy, 1, &mut typed),
            0
        );
        drop(identity);
        let mut type_name = std::ptr::null();
        assert_eq!(sn_abi_v1_resource_type(typed, &mut type_name), 0);
        assert_eq!(
            std::ffi::CStr::from_ptr(type_name).to_bytes(),
            b"pkg.RustResource@1"
        );
        let wrong = CString::new("pkg.OtherResource@1").unwrap();
        borrowed = data;
        assert_eq!(
            sn_abi_v1_resource_data_typed(typed, wrong.as_ptr(), &mut borrowed),
            4
        );
        assert_eq!(borrowed, data);
        let alias = sn_abi_v1_retain(typed);
        sn_abi_v1_release(typed);
        assert_eq!(
            sn_abi_v1_resource_data_typed(alias, type_name, &mut borrowed),
            0
        );
        assert_eq!(*(borrowed as *const i64), 42);
        sn_abi_v1_release(alias);
        assert_eq!(DESTROYED.load(Ordering::SeqCst), 2);
        #[cfg(go_bridge)]
        {
            let mut native = std::ptr::null_mut();
            assert_eq!(sn_test_go_text(&mut native), 0);
            assert_eq!(sn_abi_v1_bytes(native, &mut view), 0);
            assert_eq!(
                std::slice::from_raw_parts(view.data, view.length as usize),
                b"native Go backing"
            );
            sn_abi_v1_release(native);
            let before = sn_test_go_cleanup_count();
            assert_eq!(sn_test_go_owned_resource(&mut native), 0);
            let alias = sn_abi_v1_retain(native);
            sn_abi_v1_release(native);
            assert_eq!(sn_test_go_cleanup_count(), before);
            let credit = alias as usize;
            std::thread::spawn(move || sn_abi_v1_release(credit as *mut Value))
                .join()
                .unwrap();
            assert_eq!(sn_test_go_cleanup_count(), before + 1);
        }
        println!("shared runtime ABI: pass");
    }
}
