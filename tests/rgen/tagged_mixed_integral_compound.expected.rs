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
    let mut byte_value: u8 = 5;
    { let (__sn_rhs, __sn_place) = (2, &mut (byte_value)); let (__sn_left, __sn_right): (i64, i64) = (*__sn_place as i64, __sn_rhs as i64); let __sn_promoted = __sn_checked_0(__sn_left.checked_add(__sn_right), "Runtime error: integer overflow in addition"); let __sn_next = __sn_promoted as u8; *__sn_place = __sn_next; __sn_next };
    println!("0x{:02X}", (byte_value as u32));
    let mut int_value: i64 = 5;
    { let (__sn_rhs, __sn_place) = (2, &mut (int_value)); let (__sn_left, __sn_right): (i64, i64) = (*__sn_place as i64, __sn_rhs as i64); let __sn_promoted = __sn_checked_0(__sn_left.checked_add(__sn_right), "Runtime error: integer overflow in addition"); let __sn_next = __sn_promoted as i64; *__sn_place = __sn_next; __sn_next };
    println!("{}", int_value);
    let mut int32_value: i32 = 6;
    { let (__sn_rhs, __sn_place) = (8, &mut (int32_value)); let (__sn_left, __sn_right): (i64, i64) = (*__sn_place as i64, __sn_rhs as i64); let __sn_promoted = __sn_checked_0(__sn_left.checked_mul(__sn_right), "Runtime error: integer overflow in multiplication"); let __sn_next = __sn_promoted as i32; *__sn_place = __sn_next; __sn_next };
    println!("{}", int32_value);
}
