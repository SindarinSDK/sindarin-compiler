use std::ffi::{c_char, c_void};
use std::sync::atomic::{AtomicU64, Ordering};
#[link(name="sn_runtime_min",kind="static")]
extern "C" {
    fn sn_abi_v1_native_record_array_borrow(array:*mut c_void,rank:u32,id:*const c_char,out:*mut *mut c_void)->u32;
    fn sn_abi_v1_native_record_array_adopt(array:*mut c_void,rank:u32,id:*const c_char,out:*mut *mut c_void)->u32;
    fn sn_abi_v1_native_record_array_data(value:*mut c_void,rank:u32,id:*const c_char,out:*mut *mut c_void)->u32;
    fn sn_abi_v1_native_record_array_copy(value:*mut c_void,out:*mut *mut c_void)->u32;
    fn sn_abi_v1_native_record_array_take(value:*mut *mut c_void,rank:u32,id:*const c_char,out:*mut *mut c_void)->u32;
    fn sn_abi_v1_release(value:*mut c_void);
    fn sn_test_record_array_new()->*mut c_void;
    fn sn_test_record_array_free(array:*mut c_void);
    fn sn_test_record_array_destroyed()->u64;
    fn sn_test_record_array_value(value:*mut c_void)->i64;
    fn sn_test_record_array_reenter(value:*mut c_void,callback:unsafe extern "C" fn(*mut c_void,usize)->u32,context:usize)->u32;
}
static CALLS: AtomicU64 = AtomicU64::new(0);
unsafe extern "C" fn observe(value:*mut c_void, context:usize)->u32 {
    let count=CALLS.fetch_add(1,Ordering::SeqCst);
    assert_eq!(sn_test_record_array_value(value), count as i64+8);
    if context==99 && count==0 { sn_abi_v1_release(value); }
    0
}
fn main() { unsafe {
    let id=b"pkg.Record@1\0".as_ptr().cast();
    let original=sn_test_record_array_new();let mut view=std::ptr::null_mut();
    assert_eq!(sn_abi_v1_native_record_array_borrow(original,1,id,&mut view),0);
    let mut header=std::ptr::null_mut();
    assert_eq!(sn_abi_v1_native_record_array_data(view,1,id,&mut header),0);assert_eq!(header,original);
    let mut copy=std::ptr::null_mut();assert_eq!(sn_abi_v1_native_record_array_copy(view,&mut copy),0);
    assert_eq!(sn_test_record_array_reenter(view,observe,42),0);
    assert_eq!(sn_test_record_array_value(copy),71);
    sn_abi_v1_release(view);sn_test_record_array_free(original);assert_eq!(sn_test_record_array_destroyed(),0);
    assert_eq!(sn_abi_v1_native_record_array_take(&mut copy,1,id,&mut header),0);assert!(copy.is_null());
    sn_test_record_array_free(header);assert_eq!(sn_test_record_array_destroyed(),1);
    CALLS.store(0,Ordering::SeqCst);
    assert_eq!(sn_abi_v1_native_record_array_adopt(sn_test_record_array_new(),1,id,&mut view),0);
    assert_eq!(sn_test_record_array_reenter(view,observe,99),0);
    assert_eq!(CALLS.load(Ordering::SeqCst),64);assert_eq!(sn_test_record_array_destroyed(),2);
} println!("native record array views: pass"); }
