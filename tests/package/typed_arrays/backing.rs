use std::ffi::c_void;
extern "C" {
    fn sn_typed_copy(a: *mut c_void, b: *mut c_void, mode: i64) -> *mut c_void;
    fn sn_typed_observe(value: *mut c_void);
    fn sn_typed_free(value: *mut c_void);
    fn sn_typed_ints(a: *mut c_void, b: *mut c_void) -> *mut c_void;
}
pub fn provide_ints(a: *mut c_void, b: *mut c_void) -> Result<*mut c_void, u32> {
    unsafe { Ok(sn_typed_ints(a, b)) }
}
pub fn provide(a: *mut c_void, b: *mut c_void, mode: i64) -> Result<*mut c_void, u32> {
    if mode == 2 {
        panic!("typed provider panic");
    }
    if mode == 3 {
        return Err(5);
    }
    unsafe {
        let value = sn_typed_copy(a, b, mode);
        let observe = || sn_typed_observe(b);
        observe();
        if mode == 4 {
            sn_typed_free(value);
            return Err(5);
        }
        Ok(value)
    }
}
