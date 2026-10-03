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
struct ByteBox {
    value: u8,
}

fn bump(value: &mut u8) -> u8 {
    let mut one: u8 = 1;
    return { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (one, &mut (*(value))); let __sn_byte_next = (*__sn_byte_place).wrapping_add(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };
}

fn main() {
    let __sn_stdio_guard = __SnStdioGuard;
    let mut max: u8 = 255;
    let mut zero: u8 = 0;
    let mut one: u8 = 1;
    let mut two: u8 = 2;
    let mut added: u8 = { let (__sn_byte_left, __sn_byte_right): (u8, u8) = (max, one); __sn_byte_left.wrapping_add(__sn_byte_right) };
    let mut subtracted: u8 = { let (__sn_byte_left, __sn_byte_right): (u8, u8) = (zero, one); __sn_byte_left.wrapping_sub(__sn_byte_right) };
    let mut multiplied: u8 = { let (__sn_byte_left, __sn_byte_right): (u8, u8) = (max, two); __sn_byte_left.wrapping_mul(__sn_byte_right) };
    let mut negated: u8 = -(one as i32) as u8;
    let mut inverted: u8 = !(zero as i32) as u8;
    println!("{}", (added == 0));
    println!("{}", (subtracted == 255));
    println!("{}", (multiplied == 254));
    println!("{}", (negated == 255));
    println!("{}", (inverted == 255));
    let mut add: u8 = max;
    let mut sub: u8 = zero;
    let mut mul: u8 = max;
    let mut add_result: u8 = { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (one, &mut (add)); let __sn_byte_next = (*__sn_byte_place).wrapping_add(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };
    let mut sub_result: u8 = { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (one, &mut (sub)); let __sn_byte_next = (*__sn_byte_place).wrapping_sub(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };
    let mut mul_result: u8 = { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (two, &mut (mul)); let __sn_byte_next = (*__sn_byte_place).wrapping_mul(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };
    println!("{}", (add_result == 0));
    println!("{}", (sub_result == 255));
    println!("{}", (mul_result == 254));
    let mut inc: u8 = max;
    let mut dec: u8 = zero;
    println!("{}", ({ let __sn_byte_place = &mut (inc); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous } == 255));
    println!("{}", (inc == 0));
    println!("{}", ({ let __sn_byte_place = &mut (dec); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_sub(1); __sn_byte_previous } == 0));
    println!("{}", (dec == 255));
    let mut r#box: ByteBox = ByteBox { value: 255 };
    println!("{}", ({ let __sn_byte_place = &mut ((r#box).value); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous } == 255));
    println!("{}", ((r#box).value == 0));
    let mut referenced: u8 = max;
    println!("{}", (bump(&mut (referenced)) == 0));
    println!("{}", (referenced == 0));
    let mut shifted: u8 = { let (__sn_byte_left, __sn_byte_right): (i32, i32) = (two as i32, 8 as i32); __sn_byte_left << (__sn_byte_right as u32) } as u8;
    println!("{}", (shifted == 0));
    let mut mask: u8 = 15;
    let mut anded: u8 = { let (__sn_byte_left, __sn_byte_right): (i32, i32) = (max as i32, mask as i32); __sn_byte_left & __sn_byte_right } as u8;
    let mut ored: u8 = { let (__sn_byte_left, __sn_byte_right): (i32, i32) = (zero as i32, mask as i32); __sn_byte_left | __sn_byte_right } as u8;
    let mut xored: u8 = { let (__sn_byte_left, __sn_byte_right): (i32, i32) = (max as i32, mask as i32); __sn_byte_left ^ __sn_byte_right } as u8;
    println!("{}", (anded == 15));
    println!("{}", (ored == 15));
    println!("{}", (xored == 240));
    let mut compound_shift: u8 = two;
    { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (8, &mut (compound_shift)); let __sn_byte_next = (*__sn_byte_place as u32).wrapping_shl(__sn_byte_rhs as u32) as u8; *__sn_byte_place = __sn_byte_next; __sn_byte_next };
    println!("{}", (compound_shift == 0));
    let mut compound_bits: u8 = 240;
    { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (mask, &mut (compound_bits)); let __sn_byte_next = *__sn_byte_place | __sn_byte_rhs; *__sn_byte_place = __sn_byte_next; __sn_byte_next };
    { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (255, &mut (compound_bits)); let __sn_byte_next = *__sn_byte_place ^ __sn_byte_rhs; *__sn_byte_place = __sn_byte_next; __sn_byte_next };
    println!("{}", (compound_bits == 0));
    let mut quotient: u8 = 240;
    let mut remainder: u8 = 240;
    { let __sn_rhs = two; let __sn_place = &mut (quotient); let __sn_next = __sn_checked_div_0((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = 7; let __sn_place = &mut (remainder); let __sn_next = __sn_checked_mod_0((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    println!("{}", (quotient == 120));
    println!("{}", (remainder == 2));
    let mut __sn_byte_left: u8 = max;
    let mut __sn_byte_right: u8 = one;
    let mut hygienic_binary: u8 = { let (__sn_byte_left, __sn_byte_right): (u8, u8) = (__sn_byte_left, __sn_byte_right); __sn_byte_left.wrapping_add(__sn_byte_right) };
    println!("{}", (hygienic_binary == 0));
    let mut __sn_byte_rhs: u8 = max;
    let mut __sn_byte_place: u8 = one;
    let mut hygienic_compound: u8 = { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (__sn_byte_place, &mut (__sn_byte_rhs)); let __sn_byte_next = (*__sn_byte_place).wrapping_add(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };
    println!("{}", (hygienic_compound == 0));
}
