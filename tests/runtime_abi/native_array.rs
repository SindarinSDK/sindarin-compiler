use std::ffi::c_void;
use std::sync::atomic::{AtomicU64, Ordering};
#[repr(C)]
#[derive(Clone, Copy)]
struct ArrayType { leaf_kind: u32, rank: u32 }
#[link(name="sn_runtime_min",kind="static")]
extern "C" {
    fn sn_abi_v1_native_array_borrow(array:*mut c_void,shape:ArrayType,out:*mut *mut c_void)->u32;
    fn sn_abi_v1_native_array_adopt(array:*mut c_void,shape:ArrayType,out:*mut *mut c_void)->u32;
    fn sn_abi_v1_native_array_data(value:*mut c_void,shape:ArrayType,out:*mut *mut c_void)->u32;
    fn sn_abi_v1_native_array_copy(value:*mut c_void,out:*mut *mut c_void)->u32;
    fn sn_abi_v1_native_array_take(value:*mut *mut c_void,shape:ArrayType,out:*mut *mut c_void)->u32;
    fn sn_abi_v1_retain(value:*mut c_void)->*mut c_void;
    fn sn_abi_v1_release(value:*mut c_void);
    fn sn_test_typed_array_new()->*mut c_void;
    fn sn_test_typed_array_free(array:*mut c_void);
    fn sn_test_typed_array_length(value:*mut c_void)->u64;
    fn sn_test_typed_array_reenter(value:*mut c_void,callback:unsafe extern "C" fn(*mut c_void,usize)->u32,context:usize)->u32;
    fn sn_test_typed_array_destroyed()->u64;
}
static CALLS: AtomicU64 = AtomicU64::new(0);
unsafe extern "C" fn observe(view:*mut c_void, context:usize)->u32 {
    let count=CALLS.fetch_add(1,Ordering::SeqCst);
    assert_eq!(sn_test_typed_array_length(view),count+2);
    if context==99 && count==0 { sn_abi_v1_release(view); }
    0
}
fn main() { unsafe {
    let shape=ArrayType{leaf_kind:10,rank:2};let mut view=std::ptr::null_mut();
    let original=sn_test_typed_array_new();
    assert_eq!(sn_abi_v1_native_array_borrow(original,shape,&mut view),0);
    let alias=sn_abi_v1_retain(view);let mut header=std::ptr::null_mut();
    assert_eq!(sn_abi_v1_native_array_data(alias,shape,&mut header),0);assert_eq!(header,original);
    let mut copy=std::ptr::null_mut();assert_eq!(sn_abi_v1_native_array_copy(view,&mut copy),0);
    assert_eq!(sn_test_typed_array_reenter(view,observe,42),0);assert_eq!(CALLS.load(Ordering::SeqCst),64);
    assert_eq!(sn_test_typed_array_length(alias),65);
    sn_abi_v1_release(view);sn_abi_v1_release(alias);assert_eq!(sn_test_typed_array_destroyed(),0);
    sn_test_typed_array_free(original);assert_eq!(sn_test_typed_array_destroyed(),65);
    assert_eq!(sn_test_typed_array_length(copy),1);
    assert_eq!(sn_abi_v1_native_array_take(&mut copy,shape,&mut header),0);assert!(copy.is_null());
    sn_test_typed_array_free(header);assert_eq!(sn_test_typed_array_destroyed(),66);
    CALLS.store(0,Ordering::SeqCst);
    assert_eq!(sn_abi_v1_native_array_adopt(sn_test_typed_array_new(),shape,&mut view),0);
    assert_eq!(sn_test_typed_array_reenter(view,observe,99),0);assert_eq!(sn_test_typed_array_destroyed(),131);
} println!("typed native array views: pass"); }
