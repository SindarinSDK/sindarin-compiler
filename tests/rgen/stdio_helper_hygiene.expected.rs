#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

extern "C" {
    #[link_name = "fwrite"]
    fn __sn_stdio_fwrite_1(_: *const std::ffi::c_void, _: usize, _: usize, _: *mut std::ffi::c_void) -> usize;
    #[link_name = "fflush"]
    fn __sn_stdio_fflush_1(_: *mut std::ffi::c_void) -> std::ffi::c_int;
    #[link_name = "exit"]
    fn __sn_stdio_c_exit_1(_: std::ffi::c_int) -> !;
    #[cfg(windows)]
    #[link_name = "__acrt_iob_func"]
    fn __sn_stdio_iob_1(_: u32) -> *mut std::ffi::c_void;
    #[cfg(target_os = "macos")]
    #[link_name = "__stdoutp"]
    static mut __sn_stdio_stdout_1: *mut std::ffi::c_void;
    #[cfg(target_os = "macos")]
    #[link_name = "__stderrp"]
    static mut __sn_stdio_stderr_1: *mut std::ffi::c_void;
    #[cfg(all(not(windows), not(target_os = "macos")))]
    #[link_name = "stdout"]
    static mut __sn_stdio_stdout_1: *mut std::ffi::c_void;
    #[cfg(all(not(windows), not(target_os = "macos")))]
    #[link_name = "stderr"]
    static mut __sn_stdio_stderr_1: *mut std::ffi::c_void;
}

fn __sn_stdio_write_1(bytes: &[u8], stderr: bool) {
    unsafe {
        #[cfg(windows)]
        let stream = __sn_stdio_iob_1(if stderr { 2 } else { 1 });
        #[cfg(not(windows))]
        let stream = if stderr { __sn_stdio_stderr_1 } else { __sn_stdio_stdout_1 };
        __sn_stdio_fwrite_1(bytes.as_ptr().cast(), 1, bytes.len(), stream);
    }
}

fn __sn_stdio_exit_1(status: i32) -> ! {
    unsafe { __sn_stdio_c_exit_1(status) }
}

struct __SnStdioGuard_1;
impl Drop for __SnStdioGuard_1 {
    fn drop(&mut self) {
        unsafe { __sn_stdio_fflush_1(std::ptr::null_mut()); }
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
    __sn_stdio_write_1(bytes, false);
}

fn __sn_write_stderr_bytes(bytes: &[u8]) {
    __sn_stdio_write_1(bytes, true);
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
    crate::__sn_stdio_exit_1(1);
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

#[derive(Clone, Copy, Debug, PartialEq)]
struct __SnStdioGuard {
    value: i64,
}

fn __sn_stdio_fwrite() -> i64 {
    return 1;
}

fn __sn_stdio_fflush() -> i64 {
    return 2;
}

fn __sn_stdio_c_exit() -> i64 {
    return 3;
}

fn __sn_stdio_iob() -> i64 {
    return 4;
}

fn __sn_stdio_write() -> i64 {
    return 5;
}

fn __sn_stdio_exit() -> i64 {
    return 6;
}

fn __sn_stdio_stdout() -> i64 {
    return 7;
}

fn __sn_stdio_stderr() -> i64 {
    return 8;
}

fn main() {
    let mut __sn_stdio_guard: i64 = 10;
    let mut guard: __SnStdioGuard = __SnStdioGuard { value: 9 };
    let mut total: i64 = __sn_checked_0((__sn_stdio_fwrite()).checked_add(__sn_stdio_fflush()), "Runtime error: integer overflow in addition");
    { let __sn_rhs = __sn_checked_0((__sn_stdio_c_exit()).checked_add(__sn_stdio_iob()), "Runtime error: integer overflow in addition"); let __sn_place = &mut (total); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = __sn_checked_0((__sn_stdio_write()).checked_add(__sn_stdio_exit()), "Runtime error: integer overflow in addition"); let __sn_place = &mut (total); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = __sn_checked_0((__sn_stdio_stdout()).checked_add(__sn_stdio_stderr()), "Runtime error: integer overflow in addition"); let __sn_place = &mut (total); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    println!("{}", __sn_checked_0((__sn_checked_0((total).checked_add((guard).value), "Runtime error: integer overflow in addition")).checked_add(__sn_stdio_guard), "Runtime error: integer overflow in addition"));
    return;
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob_1(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout_1;
        crate::__sn_stdio_fflush_1(stream);
    }
}
