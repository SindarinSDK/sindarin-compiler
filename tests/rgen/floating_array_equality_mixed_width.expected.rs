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

fn main() {
    let mut narrow: Vec<f32> = vec![0.0, 0.0];
    let mut wide: Vec<f64> = vec![0.0, 1000.0];
    println!("{}", { let __sn_float_eq_right: &[f64] = &(wide); let __sn_float_eq_left: &[f32] = &(narrow); (__sn_float_eq_left.len() == __sn_float_eq_right.len() && __sn_float_eq_left.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left.len() * std::mem::size_of::<f32>()))) }
);
    println!("{}", { let __sn_float_eq_right_1: &[f64] = &(wide); let __sn_float_eq_left_1: &[f32] = &(narrow); !(__sn_float_eq_left_1.len() == __sn_float_eq_right_1.len() && __sn_float_eq_left_1.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_1.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_1.len() * std::mem::size_of::<f32>()))) }
);
    let mut emptyNarrow: Vec<f32> = vec![];
    let mut emptyWide: Vec<f64> = vec![];
    println!("{}", { let __sn_float_eq_right_2: &[f64] = &(emptyWide); let __sn_float_eq_left_2: &[f32] = &(emptyNarrow); (__sn_float_eq_left_2.len() == __sn_float_eq_right_2.len() && __sn_float_eq_left_2.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_2.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_2.len() * std::mem::size_of::<f32>()))) }
);
    println!("{}", { let __sn_float_eq_right_3: &[f32] = &(emptyNarrow); let __sn_float_eq_left_3: &[f64] = &(emptyWide); (__sn_float_eq_left_3.len() == __sn_float_eq_right_3.len() && __sn_float_eq_left_3.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_3.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_3.len() * std::mem::size_of::<f64>()))) }
);
    println!("{}", { let __sn_float_eq_right_4: &[f64] = &(emptyWide); let __sn_float_eq_left_4: &[f32] = &(narrow); (__sn_float_eq_left_4.len() == __sn_float_eq_right_4.len() && __sn_float_eq_left_4.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_4.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_4.len() * std::mem::size_of::<f32>()))) }
);
    println!("{}", { let __sn_float_eq_right_5: &[f32] = &(narrow); let __sn_float_eq_left_5: &[f64] = &(emptyWide); !(__sn_float_eq_left_5.len() == __sn_float_eq_right_5.len() && __sn_float_eq_left_5.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_5.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_5.len() * std::mem::size_of::<f64>()))) }
);
    let mut oneNarrow: Vec<f32> = vec![0.0];
    let mut oneWide: Vec<f64> = vec![0.0];
    println!("{}", { let __sn_float_eq_right_6: &[f64] = &(oneWide); let __sn_float_eq_left_6: &[f32] = &(oneNarrow); (__sn_float_eq_left_6.len() == __sn_float_eq_right_6.len() && __sn_float_eq_left_6.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_6.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_6.len() * std::mem::size_of::<f32>()))) }
);
    println!("{}", { let __sn_float_eq_right_7: &[f64] = &(oneWide); let __sn_float_eq_left_7: &[f32] = &(oneNarrow); !(__sn_float_eq_left_7.len() == __sn_float_eq_right_7.len() && __sn_float_eq_left_7.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_7.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_7.len() * std::mem::size_of::<f32>()))) }
);
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout;
        crate::__sn_stdio_fflush(stream);
    }
}
