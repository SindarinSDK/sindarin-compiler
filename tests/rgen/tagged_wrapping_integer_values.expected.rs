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
    let mut i_max: i32 = 2147483647;
    println!("{}", { let (__sn_byte_left, __sn_byte_right): (i32, i32) = (i_max, 1); __sn_byte_left.wrapping_add(__sn_byte_right) });
    let mut u32_max: u32 = 4294967295;
    let mut u32_one: u32 = 1;
    println!("{}", { let (__sn_byte_left, __sn_byte_right): (u32, u32) = (u32_max, 1); __sn_byte_left.wrapping_add(__sn_byte_right) });
    println!("{}", { let (__sn_byte_left, __sn_byte_right): (u32, u32) = (0, 1); __sn_byte_left.wrapping_sub(__sn_byte_right) });
    println!("{}", { let (__sn_byte_left, __sn_byte_right): (u32, u32) = (65536, 65536); __sn_byte_left.wrapping_mul(__sn_byte_right) });
    println!("{}", { let __sn_byte_operand: u32 = u32_one; __sn_byte_operand.wrapping_neg() });
    let mut u_half: u64 = 9223372036854775807;
    let mut u_max: u64 = { let (__sn_byte_left, __sn_byte_right): (u64, u64) = ({ let (__sn_byte_left, __sn_byte_right): (u64, u64) = (u_half, 2); __sn_byte_left.wrapping_mul(__sn_byte_right) }, 1); __sn_byte_left.wrapping_add(__sn_byte_right) };
    println!("{}", ({ let (__sn_byte_left, __sn_byte_right): (u64, u64) = (u_max, 1); __sn_byte_left.wrapping_add(__sn_byte_right) } as i64));
    println!("{}", ({ let (__sn_byte_left, __sn_byte_right): (u64, u64) = (0, 1); __sn_byte_left.wrapping_sub(__sn_byte_right) } as i64));
}
