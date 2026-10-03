#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

extern "C" {
    #[link_name = "fwrite"]
    fn __sn_stdio_fwrite(_: *const std::ffi::c_void, _: usize, _: usize, _: *mut std::ffi::c_void) -> usize;
    #[link_name = "fflush"]
    fn __sn_stdio_fflush(_: *mut std::ffi::c_void) -> std::ffi::c_int;
    #[link_name = "exit"]
    fn __sn_stdio_c_exit(_: std::ffi::c_int) -> !;
    #[cfg(windows)]
    #[link_name = "__acrt_iob_func"]
    fn __sn_stdio_iob(_: u32) -> *mut std::ffi::c_void;
    #[cfg(target_os = "macos")]
    #[link_name = "__stdoutp"]
    static mut __sn_stdio_stdout: *mut std::ffi::c_void;
    #[cfg(target_os = "macos")]
    #[link_name = "__stderrp"]
    static mut __sn_stdio_stderr: *mut std::ffi::c_void;
    #[cfg(all(not(windows), not(target_os = "macos")))]
    #[link_name = "stdout"]
    static mut __sn_stdio_stdout: *mut std::ffi::c_void;
    #[cfg(all(not(windows), not(target_os = "macos")))]
    #[link_name = "stderr"]
    static mut __sn_stdio_stderr: *mut std::ffi::c_void;
}

fn __sn_stdio_write(bytes: &[u8], stderr: bool) {
    unsafe {
        #[cfg(windows)]
        let stream = __sn_stdio_iob(if stderr { 2 } else { 1 });
        #[cfg(not(windows))]
        let stream = if stderr { __sn_stdio_stderr } else { __sn_stdio_stdout };
        __sn_stdio_fwrite(bytes.as_ptr().cast(), 1, bytes.len(), stream);
    }
}

fn __sn_stdio_exit(status: i32) -> ! {
    unsafe { __sn_stdio_c_exit(status) }
}

struct __SnStdioGuard;
impl Drop for __SnStdioGuard {
    fn drop(&mut self) {
        unsafe { __sn_stdio_fflush(std::ptr::null_mut()); }
    }
}


#[cfg(windows)]
fn __sn_write_windows_text<W: std::io::Write>(writer: &mut W, bytes: &[u8]) {
    let mut start = 0usize;
    for (index, byte) in bytes.iter().enumerate() {
        if *byte == b'\n' {
            writer.write_all(&bytes[start..index]).expect("failed to write output");
            writer.write_all(b"\r\n").expect("failed to write output newline");
            start = index + 1;
        }
    }
    writer.write_all(&bytes[start..]).expect("failed to write output");
    writer.flush().expect("failed to flush output");
}

fn __sn_write_stdout_bytes(bytes: &[u8]) {
    __sn_stdio_write(bytes, false);
}

fn __sn_write_stderr_bytes(bytes: &[u8]) {
    __sn_stdio_write(bytes, true);
}

fn __sn_print_format(arguments: std::fmt::Arguments<'_>) {
    let mut rendered = std::string::String::new();
    std::fmt::write(&mut rendered, arguments).expect("failed to format output");
    __sn_write_stdout_bytes(rendered.as_bytes());
}

fn __sn_println_format(arguments: std::fmt::Arguments<'_>) {
    let mut rendered = std::string::String::new();
    std::fmt::write(&mut rendered, arguments).expect("failed to format output");
    rendered.push('\n');
    __sn_write_stdout_bytes(rendered.as_bytes());
}

macro_rules! print {
    ($($arg:tt)*) => { crate::__sn_print_format(format_args!($($arg)*)) };
}

macro_rules! println {
    () => { crate::__sn_println_format(format_args!("")) };
    ($($arg:tt)*) => { crate::__sn_println_format(format_args!($($arg)*)) };
}


fn __sn_index(length: usize, index: i64) -> usize {
    let resolved = if index < 0 { length as i64 + index } else { index };
    if resolved < 0 || resolved >= length as i64 {
        panic!("array index out of bounds: {index}");
    }
    resolved as usize
}

fn __sn_insert_index(length: usize, index: i64) -> usize {
    let resolved = if index < 0 { length as i64 + index } else { index };
    if resolved < 0 || resolved > length as i64 {
        panic!("array insert index out of bounds: {index}");
    }
    resolved as usize
}

fn __sn_array_size(size: i64) -> usize {
    if size < 0 {
        panic!("array size cannot be negative: {size}");
    }
    size as usize
}

fn __sn_runtime_error_0(message: &'static str) -> ! {
    crate::__sn_write_stderr_bytes(&[message.as_bytes(), b"\n"].concat());
    crate::__sn_stdio_exit(1);
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

fn narrow(value: i32) -> i32 {
    return value;
}

fn main() {
    let mut zero: i32 = 0;
    let mut unsigned: Vec<u32> = vec![4294967295];
    { let __sn_numeric_old: u32 = { let __sn_place_raw_index_1 = 0; let __sn_place_index_1 = if __sn_place_raw_index_1 < 0 { __sn_place_raw_index_1 + (unsigned).len() as i64 } else { __sn_place_raw_index_1 }; (unsigned)[__sn_place_index_1 as usize] }; let __sn_numeric_rhs: u32 = ({ let (__sn_left, __sn_right): (i32, i32) = ((zero) as i32, (((2i128) as i64)) as i32); __sn_checked_0(__sn_left.checked_sub(__sn_right), "Runtime error: integer overflow in subtraction") }
) as u32; let __sn_numeric_next: u32 = ((__sn_numeric_old as u32) / __sn_numeric_rhs) as u32; let __sn_place_raw_index_0 = 0; let __sn_place_index_0 = if __sn_place_raw_index_0 < 0 { __sn_place_raw_index_0 + (unsigned).len() as i64 } else { __sn_place_raw_index_0 }; let __sn_numeric_place: &mut u32 = &mut ((unsigned)[__sn_place_index_0 as usize]); *__sn_numeric_place = __sn_numeric_next; __sn_numeric_next };
    println!("{}", (unsigned)[__sn_index((unsigned).len(), 0)]);
    let mut floatZero: f32 = 0.0;
    let mut values: Vec<f32> = vec![1.0];
    { let __sn_numeric_old_1: f32 = { let __sn_place_raw_index_3 = 0; let __sn_place_index_3 = if __sn_place_raw_index_3 < 0 { __sn_place_raw_index_3 + (values).len() as i64 } else { __sn_place_raw_index_3 }; (values)[__sn_place_index_3 as usize] }; let __sn_numeric_rhs_1: f32 = (((((1.0000000600000001f64)) as f32) + ((floatZero) as f32))) as f32; let __sn_numeric_next_1: f32 = ((__sn_numeric_old_1 as f32) - __sn_numeric_rhs_1) as f32; let __sn_place_raw_index_2 = 0; let __sn_place_index_2 = if __sn_place_raw_index_2 < 0 { __sn_place_raw_index_2 + (values).len() as i64 } else { __sn_place_raw_index_2 }; let __sn_numeric_place_1: &mut f32 = &mut ((values)[__sn_place_index_2 as usize]); *__sn_numeric_place_1 = __sn_numeric_next_1; __sn_numeric_next_1 };
    println!("{}", (((((values)[__sn_index((values).len(), 0)]) as f32) > (((-7.0000000000000005e-08)) as f32)) && ((((values)[__sn_index((values).len(), 0)]) as f32) < (((-4.9999999999999998e-08)) as f32))));
    let mut callResult: Vec<u32> = vec![4294967295];
    { let __sn_numeric_old_2: u32 = { let __sn_place_raw_index_5 = 0; let __sn_place_index_5 = if __sn_place_raw_index_5 < 0 { __sn_place_raw_index_5 + (callResult).len() as i64 } else { __sn_place_raw_index_5 }; (callResult)[__sn_place_index_5 as usize] }; let __sn_numeric_rhs_2: u32 = (narrow({ let (__sn_left, __sn_right): (i32, i32) = ((zero) as i32, (((2i128) as i64)) as i32); __sn_checked_0(__sn_left.checked_sub(__sn_right), "Runtime error: integer overflow in subtraction") }
)) as u32; let __sn_numeric_next_2: u32 = ((__sn_numeric_old_2 as u32) / __sn_numeric_rhs_2) as u32; let __sn_place_raw_index_4 = 0; let __sn_place_index_4 = if __sn_place_raw_index_4 < 0 { __sn_place_raw_index_4 + (callResult).len() as i64 } else { __sn_place_raw_index_4 }; let __sn_numeric_place_2: &mut u32 = &mut ((callResult)[__sn_place_index_4 as usize]); *__sn_numeric_place_2 = __sn_numeric_next_2; __sn_numeric_next_2 };
    println!("{}", (callResult)[__sn_index((callResult).len(), 0)]);
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout;
        crate::__sn_stdio_fflush(stream);
    }
}
