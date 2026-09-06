#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

unsafe extern "C" {
    #[cfg(not(windows))]
    #[cfg_attr(target_vendor = "apple", link_name = "__stdoutp")]
    #[cfg_attr(not(target_vendor = "apple"), link_name = "stdout")]
    static mut __sn_c_stdout: *mut std::ffi::c_void;

    #[cfg(windows)]
    #[link_name = "__acrt_iob_func"]
    fn __sn_c_stdout(index: u32) -> *mut std::ffi::c_void;

    #[link_name = "fwrite"]
    fn __sn_c_fwrite(
        data: *const std::ffi::c_void,
        size: usize,
        count: usize,
        stream: *mut std::ffi::c_void,
    ) -> usize;
}

fn __sn_stdout_stream() -> *mut std::ffi::c_void {
    unsafe {
        #[cfg(windows)]
        { __sn_c_stdout(1) }
        #[cfg(not(windows))]
        { __sn_c_stdout }
    }
}

fn __sn_stdout_write(bytes: &[u8]) {
    if bytes.is_empty() { return; }
    let written = unsafe {
        __sn_c_fwrite(
            bytes.as_ptr().cast(), 1, bytes.len(),
            __sn_stdout_stream())
    };
    if written != bytes.len() { panic!("failed to write stdout"); }
}

macro_rules! print {
    ($($arg:tt)*) => {
        let rendered = format!($($arg)*);
        __sn_stdout_write(rendered.as_bytes());
    };
}

macro_rules! println {
    () => { __sn_stdout_write(b"\n") };
    ($($arg:tt)*) => {
        let mut rendered = format!($($arg)*);
        rendered.push('\n');
        __sn_stdout_write(rendered.as_bytes());
    };
}


struct __SnClosure<F: ?Sized>(std::rc::Rc<F>);
impl<F: ?Sized> Clone for __SnClosure<F> {
    fn clone(&self) -> Self { Self(self.0.clone()) }
}
impl<F: ?Sized> std::fmt::Debug for __SnClosure<F> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("<function>")
    }
}
impl<F: ?Sized> PartialEq for __SnClosure<F> {
    fn eq(&self, other: &Self) -> bool { std::rc::Rc::ptr_eq(&self.0, &other.0) }
}
fn main() {
    let mut value: f32 = 1.0;
    let mut action: __SnClosure<dyn Fn() -> f32> = { let (value, ) = (value.clone(), ); self::__SnClosure::<dyn Fn() -> f32>(std::rc::Rc::new(move || -> f32 { let mut value = value; (value = 2.0);return value.clone();})) }
;
    println!("{:.5}", ((action.clone()).0)());
}
