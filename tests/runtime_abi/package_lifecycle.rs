use std::sync::atomic::{AtomicUsize, Ordering};
#[repr(C)] struct Package { _private: [u8; 0] }
#[repr(C)] struct Call { _private: [u8; 0] }
#[link(name="sn_runtime_min", kind="static")]
extern "C" {
    fn sn_abi_v1_package_new(init: Option<unsafe extern "C" fn(*mut Package,usize)->u32>,
        cleanup: Option<unsafe extern "C" fn(*mut Package,usize)>, context:usize, out:*mut *mut Package)->u32;
    fn sn_abi_v1_package_begin(package:*mut Package,out:*mut *mut Call)->u32;
    fn sn_abi_v1_package_end(call:*mut Call);
    fn sn_abi_v1_package_shutdown(package:*mut Package)->u32;
    fn sn_abi_v1_package_release(package:*mut Package);
}
static INIT: AtomicUsize = AtomicUsize::new(0);
static CLEANUP: AtomicUsize = AtomicUsize::new(0);
unsafe extern "C" fn initialize(package:*mut Package,context:usize)->u32 {
    assert_eq!(context,42); assert_eq!(INIT.fetch_add(1,Ordering::SeqCst),0);
    let mut call=std::ptr::null_mut(); assert_eq!(sn_abi_v1_package_begin(package,&mut call),0);
    assert_eq!(sn_abi_v1_package_shutdown(package),7); sn_abi_v1_package_end(call); 0
}
unsafe extern "C" fn cleanup(package:*mut Package,context:usize) {
    assert_eq!(context,42); assert_eq!(CLEANUP.fetch_add(1,Ordering::SeqCst),0);
    assert_eq!(sn_abi_v1_package_shutdown(package),0);
}
fn main() { unsafe {
    let mut package=std::ptr::null_mut();
    assert_eq!(sn_abi_v1_package_new(Some(initialize),Some(cleanup),42,&mut package),0);
    let mut call=std::ptr::null_mut(); assert_eq!(sn_abi_v1_package_begin(package,&mut call),0);
    sn_abi_v1_package_release(package); assert_eq!(CLEANUP.load(Ordering::SeqCst),0);
    sn_abi_v1_package_end(call); assert_eq!(CLEANUP.load(Ordering::SeqCst),1);
    println!("package lifecycle: pass");
} }
