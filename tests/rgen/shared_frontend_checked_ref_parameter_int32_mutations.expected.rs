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

#[derive(Clone, Copy, Debug, PartialEq)]
struct Box {
    value: i32,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Wrapper {
    r#box: Box,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct RefOps {
}

impl RefOps {
    fn staticOps(value: &mut i32) -> i32 {
        let mut add: i32 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
        let mut sub: i32 = { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
        let mut mul: i32 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
        let mut div: i32 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_div_0((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
        let mut rem: i32 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_mod_0((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
        let mut old_inc: i32 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        let mut old_dec: i32 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
        return { let (__sn_byte_left, __sn_byte_right): (i32, i32) = ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = (add, sub); __sn_byte_left.wrapping_add(__sn_byte_right) }, mul); __sn_byte_left.wrapping_add(__sn_byte_right) }, div); __sn_byte_left.wrapping_add(__sn_byte_right) }, rem); __sn_byte_left.wrapping_add(__sn_byte_right) }, old_inc); __sn_byte_left.wrapping_add(__sn_byte_right) }, old_dec); __sn_byte_left.wrapping_add(__sn_byte_right) }, *(value)); __sn_byte_left.wrapping_add(__sn_byte_right) };
    }
    fn staticBump(value: &mut i32) {
        { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    }
    fn instancePostfix(&self, value: &mut i32) -> i32 {
        return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    }
    fn instanceBump(&self, value: &mut i32) {
        { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    }
}

fn oldThenIncrement(value: &mut i32) -> i32 {
    return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
}

fn freeOps(value: &mut i32) -> i32 {
    let mut add: i32 = { let __sn_ref_previous = *(value); let __sn_rhs = oldThenIncrement(&mut *(value)); let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((__sn_ref_previous).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    let mut sub: i32 = { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
    let mut mul: i32 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
    let mut div: i32 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_div_0((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    let mut rem: i32 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_mod_0((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    let mut old_inc: i32 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    let mut old_dec: i32 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    return { let (__sn_byte_left, __sn_byte_right): (i32, i32) = ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = (add, sub); __sn_byte_left.wrapping_add(__sn_byte_right) }, mul); __sn_byte_left.wrapping_add(__sn_byte_right) }, div); __sn_byte_left.wrapping_add(__sn_byte_right) }, rem); __sn_byte_left.wrapping_add(__sn_byte_right) }, old_inc); __sn_byte_left.wrapping_add(__sn_byte_right) }, old_dec); __sn_byte_left.wrapping_add(__sn_byte_right) }, *(value)); __sn_byte_left.wrapping_add(__sn_byte_right) };
}

fn forwardStatic(value: &mut i32) -> i32 {
    RefOps::staticBump(&mut *(value));
    return *(value);
}

fn forwardInstance(value: &mut i32) -> i32 {
    let mut ops: RefOps = RefOps {  };
    (ops).instanceBump(&mut *(value));
    return *(value);
}

fn maxBoundary(value: &mut i32) -> i32 {
    return { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
}

fn minBoundary(value: &mut i32) -> i32 {
    return { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
}

fn read(value: &mut i32) -> i32 {
    return *(value);
}

fn freeBump(value: &mut i32) {
    { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
}

fn main() {
    let mut free_value: i32 = 2;
    let mut static_value: i32 = 10;
    let mut postfix_value: i32 = 5;
    let mut forwarded_static: i32 = 1;
    let mut forwarded_instance: i32 = 1;
    let mut maximum: i32 = 2147483646;
    let mut minimum: i32 = (-2147483647);
    let mut direct: Box = Box { value: 1 };
    let mut nested: Wrapper = Wrapper { r#box: Box { value: 10 } };
    let mut ops: RefOps = RefOps {  };
    println!("{}", freeOps(&mut (free_value)));
    println!("{}", free_value);
    println!("{}", RefOps::staticOps(&mut (static_value)));
    println!("{}", static_value);
    println!("{}", (ops).instancePostfix(&mut (postfix_value)));
    println!("{}", postfix_value);
    println!("{}", forwardStatic(&mut (forwarded_static)));
    println!("{}", forwarded_static);
    println!("{}", forwardInstance(&mut (forwarded_instance)));
    println!("{}", forwarded_instance);
    println!("{}", maxBoundary(&mut (maximum)));
    println!("{}", minBoundary(&mut (minimum)));
    freeBump(&mut ((direct).value));
    freeBump(&mut (((nested).r#box).value));
    RefOps::staticBump(&mut ((direct).value));
    RefOps::staticBump(&mut (((nested).r#box).value));
    (ops).instanceBump(&mut ((direct).value));
    (ops).instanceBump(&mut (((nested).r#box).value));
    println!("{}", read(&mut ((direct).value)));
    println!("{}", (direct).value);
    println!("{}", ((nested).r#box).value);
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout;
        crate::__sn_stdio_fflush(stream);
    }
}
