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


fn main() {
    println!("0x{:02X}", ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = (255 as i32, 1 as i32); __sn_byte_left + __sn_byte_right } as u32));
    println!("0x{:02X}", (-(1 as i32) as u32));
    println!("0x{:02X}", (!(1 as i32) as u32));
    println!("{}", ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = (255 as i32, 1 as i32); __sn_byte_left + __sn_byte_right } as i32 == 0 as i32));
    let mut stored: u8 = { let (__sn_byte_left, __sn_byte_right): (i32, i32) = ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = (255 as i32, 1 as i32); __sn_byte_left + __sn_byte_right } as i32, 2 as i32); __sn_byte_left / __sn_byte_right } as u8;
    println!("0x{:02X}", (stored as u32));
}
