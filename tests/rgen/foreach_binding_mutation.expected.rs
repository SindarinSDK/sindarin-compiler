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

fn main() {
    let mut ints: Vec<i64> = vec![1];
    for mut item in (ints).iter().cloned() {
        (item = 2);
        println!("{}", ((item == 2) && ((ints)[__sn_index((ints).len(), 0)] == 1)));
    }
    let mut longs: Vec<i64> = vec![3];
    for mut item in (longs).iter().cloned() {
        (item = 4);
        println!("{}", ((item == 4) && ((longs)[__sn_index((longs).len(), 0)] == 3)));
    }
    let mut int32s: Vec<i32> = vec![5];
    for mut item in (int32s).iter().cloned() {
        (item = 6);
        println!("{}", ((item == 6) && ((int32s)[__sn_index((int32s).len(), 0)] == 5)));
    }
    let mut uints: Vec<u64> = vec![7];
    for mut item in (uints).iter().cloned() {
        (item = 8);
        println!("{}", ((item == 8) && ((uints)[__sn_index((uints).len(), 0)] == 7)));
    }
    let mut uint32s: Vec<u32> = vec![9];
    for mut item in (uint32s).iter().cloned() {
        (item = 10);
        println!("{}", ((item == 10) && ((uint32s)[__sn_index((uint32s).len(), 0)] == 9)));
    }
    let mut bytes: Vec<u8> = vec![11];
    for mut item in (bytes).iter().cloned() {
        (item = 12);
        println!("{}", ((item == 12) && ((bytes)[__sn_index((bytes).len(), 0)] == 11)));
    }
    let mut floats: Vec<f32> = vec![1.5];
    for mut item in (floats).iter().cloned() {
        (item = 2.5);
        println!("{}", ((item == 2.5) && ((floats)[__sn_index((floats).len(), 0)] == 1.5)));
    }
    let mut doubles: Vec<f64> = vec![3.5];
    for mut item in (doubles).iter().cloned() {
        (item = 4.5);
        println!("{}", ((item == 4.5) && ((doubles)[__sn_index((doubles).len(), 0)] == 3.5)));
    }
    let mut bools: Vec<bool> = vec![true];
    for mut item in (bools).iter().cloned() {
        (item = false);
        println!("{}", ((item == false) && ((bools)[__sn_index((bools).len(), 0)] == true)));
    }
    let mut chars: Vec<char> = vec!['\u{61}'];
    for mut item in (chars).iter().cloned() {
        (item = '\u{62}');
        println!("{}", ((item == '\u{62}') && ((chars)[__sn_index((chars).len(), 0)] == '\u{61}')));
    }
    let mut range_sum: i64 = 0;
    for mut value in ((2..5).collect::<Vec<i64>>()).iter().cloned() {
        (value = __sn_checked_0((value).checked_add(1), "Runtime error: integer overflow in addition"));
        if (value == 4) {
        continue;
    }
        (range_sum = __sn_checked_0((range_sum).checked_add(value), "Runtime error: integer overflow in addition"));
    }
    println!("{}", (range_sum == 8));
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout;
        crate::__sn_stdio_fflush(stream);
    }
}
