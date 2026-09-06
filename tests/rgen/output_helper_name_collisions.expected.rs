#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

unsafe extern "C" {
    #[cfg(not(windows))]
    #[cfg_attr(target_vendor = "apple", link_name = "__stdoutp")]
    #[cfg_attr(not(target_vendor = "apple"), link_name = "stdout")]
    static mut __sn_c_stdout_1: *mut std::ffi::c_void;

    #[cfg(windows)]
    #[link_name = "__acrt_iob_func"]
    fn __sn_c_stdout_1(index: u32) -> *mut std::ffi::c_void;

    #[link_name = "fwrite"]
    fn __sn_c_fwrite_1(
        data: *const std::ffi::c_void,
        size: usize,
        count: usize,
        stream: *mut std::ffi::c_void,
    ) -> usize;
}

fn __sn_stdout_stream_1() -> *mut std::ffi::c_void {
    unsafe {
        #[cfg(windows)]
        { __sn_c_stdout_1(1) }
        #[cfg(not(windows))]
        { __sn_c_stdout_1 }
    }
}

fn __sn_stdout_write_1(bytes: &[u8]) {
    if bytes.is_empty() { return; }
    let written = unsafe {
        __sn_c_fwrite_1(
            bytes.as_ptr().cast(), 1, bytes.len(),
            __sn_stdout_stream_1())
    };
    if written != bytes.len() { panic!("failed to write stdout"); }
}

macro_rules! print {
    ($($arg:tt)*) => {
        let rendered = format!($($arg)*);
        __sn_stdout_write_1(rendered.as_bytes());
    };
}

macro_rules! println {
    () => { __sn_stdout_write_1(b"\n") };
    ($($arg:tt)*) => {
        let mut rendered = format!($($arg)*);
        rendered.push('\n');
        __sn_stdout_write_1(rendered.as_bytes());
    };
}


fn __sn_runtime_error_0(message: &'static str) -> ! {
    eprintln!("{}", message);
    std::process::exit(1);
}

fn __sn_checked_0<T>(value: Option<T>, message: &'static str) -> T {
    match value {
        Some(value) => value,
        None => __sn_runtime_error_0(message),
    }
}

fn __sn_checked_div_0<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked_0(value, if divisor_is_zero {
        "panic: Division by zero"
    } else {
        "Runtime error: integer overflow in division"
    })
}

fn __sn_checked_mod_0<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked_0(value, if divisor_is_zero {
        "panic: Modulo by zero"
    } else {
        "Runtime error: integer overflow in modulo"
    })
}

fn main() {
    let mut __sn_c_stdout: i64 = 1;
    let mut __sn_c_fwrite: i64 = 2;
    let mut __sn_stdout_stream: i64 = 4;
    let mut __sn_stdout_write: i64 = 8;
    println!("{}", __sn_checked_0((__sn_checked_0((__sn_checked_0((__sn_c_stdout).checked_add(__sn_c_fwrite), "Runtime error: integer overflow in addition")).checked_add(__sn_stdout_stream), "Runtime error: integer overflow in addition")).checked_add(__sn_stdout_write), "Runtime error: integer overflow in addition"));
}
