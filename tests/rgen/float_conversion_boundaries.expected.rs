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

#[derive(Clone, Copy, Debug, PartialEq)]
struct FloatBox {
    value: f32,
    wide: f64,
}

fn nextDouble(calls: &mut i64) -> f64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return 16777217.0;
}

fn narrow(value: f64) -> f32 {
    return ((value) as f32);
}

fn widen(value: f32) -> f64 {
    return ((value) as f64);
}

fn acceptFloat(value: f32) {
    println!("{:.5}", value);
}

fn acceptDouble(value: f64) {
    println!("{:.5}", value);
}

fn main() {
    let __sn_stdio_guard = __SnStdioGuard;
    let mut calls: i64 = 0;
    let mut base: f32 = ((16777216.0) as f32);
    let mut delta: f64 = 1.0;
    let mut local: f32 = (((((base) as f64) + ((delta) as f64))) as f32);
    println!("{:.5}", local);
    (local = ((nextDouble(&mut (calls))) as f32));
    println!("{:.5}", local);
    println!("{}", calls);
    println!("{:.5}", narrow(nextDouble(&mut (calls))));
    println!("{:.5}", widen(base));
    acceptFloat(((nextDouble(&mut (calls))) as f32));
    acceptDouble(((base) as f64));
    println!("{}", calls);
    let mut r#box: FloatBox = FloatBox { value: ((nextDouble(&mut (calls))) as f32), wide: ((base) as f64) };
    println!("{:.5}", (r#box).value);
    println!("{:.5}", (r#box).wide);
    ((r#box).value = ((nextDouble(&mut (calls))) as f32));
    ((r#box).wide = ((base) as f64));
    println!("{:.5}", (r#box).value);
    println!("{:.5}", (r#box).wide);
    println!("{}", calls);
    let mut values: Vec<f32> = vec![((nextDouble(&mut (calls))) as f32), (((((base) as f64) + ((delta) as f64))) as f32)];
    { let __sn_array_index = __sn_index((values).len(), 1); (values)[__sn_array_index] = ((nextDouble(&mut (calls))) as f32); };
    println!("{:.5}", (values)[__sn_index((values).len(), 0)]);
    println!("{:.5}", (values)[__sn_index((values).len(), 1)]);
    println!("{}", calls);
    let mut defaults: Vec<f32> = { let mut __sn_sized_values: Vec<f32> = Vec::with_capacity(__sn_array_size(2)); let mut __sn_sized_index: i64 = 0; while __sn_sized_index < (2) { __sn_sized_values.push(((nextDouble(&mut (calls))) as f32)); __sn_sized_index += 1; } __sn_sized_values };
    println!("{:.5}", (defaults)[__sn_index((defaults).len(), 0)]);
    println!("{:.5}", (defaults)[__sn_index((defaults).len(), 1)]);
    println!("{}", calls);
}
