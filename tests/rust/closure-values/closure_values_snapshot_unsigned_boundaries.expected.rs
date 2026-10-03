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
    let mut add32: u32 = 4294967295;
    let mut sub32: u32 = 0;
    let mut mul32: u32 = 2147483648;
    let mut div32: u32 = 4294967295;
    let mut mod32: u32 = 4294967295;
    let mut inc32: u32 = 4294967295;
    let mut dec32: u32 = 0;
    let mut max: u64 = (!(0 as i64) as u64);
    let mut high: u64 = { let (__sn_byte_left, __sn_byte_right): (u64, u64) = ((max / 2), 1); __sn_byte_left.wrapping_add(__sn_byte_right) };
    let mut add: u64 = max;
    let mut sub: u64 = 0;
    let mut mul: u64 = high;
    let mut div: u64 = max;
    let mut r#mod: u64 = max;
    let mut inc: u64 = max;
    let mut dec: u64 = 0;
    let mut boundaries: __SnClosure<dyn Fn() -> bool> = { let (add32, sub32, mul32, div32, mod32, inc32, dec32, add, sub, mul, div, r#mod, inc, dec, max, ) = (add32.clone(), sub32.clone(), mul32.clone(), div32.clone(), mod32.clone(), inc32.clone(), dec32.clone(), add.clone(), sub.clone(), mul.clone(), div.clone(), r#mod.clone(), inc.clone(), dec.clone(), max.clone(), ); self::__SnClosure::<dyn Fn() -> bool>(std::rc::Rc::new(move || -> bool { let mut add32 = add32; let mut sub32 = sub32; let mut mul32 = mul32; let mut div32 = div32; let mut mod32 = mod32; let mut inc32 = inc32; let mut dec32 = dec32; let mut add = add; let mut sub = sub; let mut mul = mul; let mut div = div; let mut r#mod = r#mod; let mut inc = inc; let mut dec = dec; { let (__sn_byte_rhs, __sn_byte_place): (u32, &mut u32) = (1, &mut (add32)); let __sn_byte_next = (*__sn_byte_place).wrapping_add(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };{ let (__sn_byte_rhs, __sn_byte_place): (u32, &mut u32) = (1, &mut (sub32)); let __sn_byte_next = (*__sn_byte_place).wrapping_sub(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };{ let (__sn_byte_rhs, __sn_byte_place): (u32, &mut u32) = (2, &mut (mul32)); let __sn_byte_next = (*__sn_byte_place).wrapping_mul(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };{ let (__sn_byte_rhs, __sn_byte_place): (u32, &mut u32) = (2, &mut (div32)); let __sn_byte_next = *__sn_byte_place / __sn_byte_rhs; *__sn_byte_place = __sn_byte_next; __sn_byte_next };{ let (__sn_byte_rhs, __sn_byte_place): (u32, &mut u32) = (2, &mut (mod32)); let __sn_byte_next = *__sn_byte_place % __sn_byte_rhs; *__sn_byte_place = __sn_byte_next; __sn_byte_next };let mut inc32Before: u32 = { let __sn_byte_place = &mut (inc32); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous };let mut dec32Before: u32 = { let __sn_byte_place = &mut (dec32); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_sub(1); __sn_byte_previous };{ let (__sn_byte_rhs, __sn_byte_place): (u64, &mut u64) = (1, &mut (add)); let __sn_byte_next = (*__sn_byte_place).wrapping_add(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };{ let (__sn_byte_rhs, __sn_byte_place): (u64, &mut u64) = (1, &mut (sub)); let __sn_byte_next = (*__sn_byte_place).wrapping_sub(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };{ let (__sn_byte_rhs, __sn_byte_place): (u64, &mut u64) = (2, &mut (mul)); let __sn_byte_next = (*__sn_byte_place).wrapping_mul(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };{ let (__sn_byte_rhs, __sn_byte_place): (u64, &mut u64) = (2, &mut (div)); let __sn_byte_next = *__sn_byte_place / __sn_byte_rhs; *__sn_byte_place = __sn_byte_next; __sn_byte_next };{ let (__sn_byte_rhs, __sn_byte_place): (u64, &mut u64) = (2, &mut (r#mod)); let __sn_byte_next = *__sn_byte_place % __sn_byte_rhs; *__sn_byte_place = __sn_byte_next; __sn_byte_next };let mut incBefore: u64 = { let __sn_byte_place = &mut (inc); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous };let mut decBefore: u64 = { let __sn_byte_place = &mut (dec); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_sub(1); __sn_byte_previous };let mut result: bool = ((add32.clone() == 0) && (sub32.clone() == 4294967295));(result = (((result && (mul32.clone() == 0)) && (div32.clone() == 2147483647)) && (mod32.clone() == 1)));(result = ((result && (inc32Before == 4294967295)) && (inc32.clone() == 0)));(result = ((result && (dec32Before == 0)) && (dec32.clone() == 4294967295)));(result = ((result && (add.clone() == 0)) && (sub.clone() == max.clone())));(result = (((result && (mul.clone() == 0)) && (div.clone() == (max.clone() / 2))) && (r#mod.clone() == 1)));(result = ((result && (incBefore == max.clone())) && (inc.clone() == 0)));(result = ((result && (decBefore == 0)) && (dec.clone() == max.clone())));return result;})) }
;
    println!("{}", ((boundaries.clone()).0)());
    println!("{}", ((boundaries.clone()).0)());
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout;
        crate::__sn_stdio_fflush(stream);
    }
}
