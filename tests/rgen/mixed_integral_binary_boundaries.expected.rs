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

fn nextByte(calls: &mut i64) -> u8 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return 255;
}

fn nextInt(calls: &mut i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return 1000;
}

fn main() {
    let __sn_stdio_guard = __SnStdioGuard;
    let mut b: u8 = 255;
    let mut n: i64 = 1000;
    println!("{}", { let (__sn_left, __sn_right): (i64, i64) = ((b) as i64, (n) as i64); __sn_checked_0(__sn_left.checked_add(__sn_right), "Runtime error: integer overflow in addition") }
);
    println!("{}", { let (__sn_left, __sn_right): (i64, i64) = ((n) as i64, (b) as i64); __sn_checked_0(__sn_left.checked_add(__sn_right), "Runtime error: integer overflow in addition") }
);
    println!("{}", { let (__sn_left, __sn_right): (i64, i64) = ((n) as i64, (b) as i64); __sn_checked_0(__sn_left.checked_sub(__sn_right), "Runtime error: integer overflow in subtraction") }
);
    println!("{}", { let (__sn_left, __sn_right): (i64, i64) = ((b) as i64, (n) as i64); __sn_checked_0(__sn_left.checked_mul(__sn_right), "Runtime error: integer overflow in multiplication") }
);
    println!("{}", { let (__sn_left, __sn_right): (i64, i64) = ((n) as i64, (b) as i64); __sn_checked_div_0(__sn_left.checked_div(__sn_right), __sn_right == 0) }
);
    println!("{}", { let (__sn_left, __sn_right): (i64, i64) = ((n) as i64, (b) as i64); __sn_checked_mod_0(__sn_left.checked_rem(__sn_right), __sn_right == 0) }
);
    let mut negative: i64 = (-1);
    let mut one: u64 = 1;
    println!("{}", { let (__sn_left, __sn_right): (u64, u64) = ((negative) as u64, (one) as u64); __sn_left == __sn_right }
);
    println!("{}", { let (__sn_left, __sn_right): (u64, u64) = ((negative) as u64, (one) as u64); __sn_left != __sn_right }
);
    println!("{}", { let (__sn_left, __sn_right): (u64, u64) = ((negative) as u64, (one) as u64); __sn_left <= __sn_right }
);
    println!("{}", { let (__sn_left, __sn_right): (u64, u64) = ((negative) as u64, (one) as u64); __sn_left >= __sn_right }
);
    println!("{}", { let (__sn_left, __sn_right): (i64, i64) = ((negative) as i64, (one) as i64); __sn_left < __sn_right }
);
    println!("{}", { let (__sn_left, __sn_right): (u64, u64) = ((one) as u64, (negative) as u64); __sn_left > __sn_right }
);
    println!("{}", ({ let (__sn_left, __sn_right): (u64, u64) = ((negative) as u64, (one) as u64); __sn_left.wrapping_add(__sn_right) }
 as i64));
    let mut signed32: i32 = (-1);
    let mut unsigned32: u32 = 4294967295;
    println!("{}", { let (__sn_left, __sn_right): (u32, u32) = ((signed32) as u32, (unsigned32) as u32); __sn_left == __sn_right }
);
    println!("{}", { let (__sn_left, __sn_right): (i32, i32) = ((signed32) as i32, (unsigned32) as i32); __sn_left < __sn_right }
);
    println!("{}", { let (__sn_left, __sn_right): (i64, i64) = ((unsigned32) as i64, (b) as i64); __sn_checked_0(__sn_left.checked_add(__sn_right), "Runtime error: integer overflow in addition") }
);
    println!("{}", { let (__sn_left, __sn_right): (i64, i64) = ((unsigned32) as i64, (b) as i64); __sn_checked_0(__sn_left.checked_mul(__sn_right), "Runtime error: integer overflow in multiplication") }
);
    println!("{}", { let (__sn_left, __sn_right): (i64, i64) = ((unsigned32) as i64, (b) as i64); __sn_checked_div_0(__sn_left.checked_div(__sn_right), __sn_right == 0) }
);
    println!("{}", { let (__sn_left, __sn_right): (i64, i64) = ((unsigned32) as i64, (b) as i64); __sn_checked_mod_0(__sn_left.checked_rem(__sn_right), __sn_right == 0) }
);
    println!("{}", { let (__sn_left, __sn_right): (u32, u32) = ((n) as u32, (unsigned32) as u32); __sn_left.wrapping_add(__sn_right) }
);
    let mut wrapped: i64 = (({ let (__sn_byte_left, __sn_byte_right): (u8, u8) = (b, b); __sn_byte_left.wrapping_mul(__sn_byte_right) }) as i64);
    println!("{}", wrapped);
    println!("{}", { let (__sn_left, __sn_right): (i64, i64) = (({ let (__sn_byte_left, __sn_byte_right): (u8, u8) = (b, b); __sn_byte_left.wrapping_mul(__sn_byte_right) }) as i64, (n) as i64); __sn_checked_0(__sn_left.checked_add(__sn_right), "Runtime error: integer overflow in addition") }
);
    println!("{}", { let (__sn_left, __sn_right): (i64, i64) = (({ let (__sn_byte_left, __sn_byte_right): (u8, u8) = (b, b); __sn_byte_left.wrapping_mul(__sn_byte_right) }) as i64, (n) as i64); __sn_left == __sn_right }
);
    let mut wide: i64 = 4295032321;
    println!("{}", { let (__sn_left, __sn_right): (i64, i64) = (({ let (__sn_byte_left, __sn_byte_right): (u8, u8) = (b, b); __sn_byte_left.wrapping_mul(__sn_byte_right) }) as i64, (wide) as i64); __sn_left == __sn_right }
);
    println!("{}", { let (__sn_left, __sn_right): (u8, u8) = (({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = (b as i32, 1 as i32); __sn_byte_left << (__sn_byte_right as u32) }) as u8, (n) as u8); __sn_left > __sn_right }
);
    let mut widened: i64 = ((b) as i64);
    println!("{}", widened);
    let mut __sn_left: i64 = 7;
    let mut __sn_right: u8 = 3;
    println!("{}", { let (__sn_left, __sn_right): (i64, i64) = ((__sn_left) as i64, (__sn_right) as i64); __sn_checked_0(__sn_left.checked_add(__sn_right), "Runtime error: integer overflow in addition") }
);
    let mut byteCalls: i64 = 0;
    let mut intCalls: i64 = 0;
    println!("{}", { let (__sn_left, __sn_right): (i64, i64) = ((nextByte(&mut (byteCalls))) as i64, (nextInt(&mut (intCalls))) as i64); __sn_checked_0(__sn_left.checked_add(__sn_right), "Runtime error: integer overflow in addition") }
);
    println!("{}", byteCalls);
    println!("{}", intCalls);
}
