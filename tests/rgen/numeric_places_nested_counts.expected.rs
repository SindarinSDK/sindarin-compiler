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
struct Inner {
    value: i64,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Outer {
    inner: Inner,
}

fn index(calls: &mut i64, result: i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return result;
}

fn main() {
    let __sn_stdio_guard = __SnStdioGuard;
    let mut calls: i64 = 0;
    let mut rows: Vec<Vec<i64>> = vec![vec![10, 20], vec![30, 40]];
    println!("{}", { let __sn_numeric_old: i64 = { let __sn_place_raw_index_3 = index(&mut (calls), 1); let __sn_place_index_3 = if __sn_place_raw_index_3 < 0 { __sn_place_raw_index_3 + (rows).len() as i64 } else { __sn_place_raw_index_3 }; let __sn_place_raw_index_4 = index(&mut (calls), 0); let __sn_place_index_4 = if __sn_place_raw_index_4 < 0 { let __sn_place_raw_index_5 = index(&mut (calls), 1); let __sn_place_index_5 = if __sn_place_raw_index_5 < 0 { __sn_place_raw_index_5 + (rows).len() as i64 } else { __sn_place_raw_index_5 }; __sn_place_raw_index_4 + ((rows)[__sn_place_index_5 as usize]).len() as i64 } else { __sn_place_raw_index_4 }; ((rows)[__sn_place_index_3 as usize])[__sn_place_index_4 as usize] }; let __sn_numeric_rhs: i64 = (((5i128) as i64)) as i64; let __sn_numeric_next: i64 = (__sn_numeric_old as i64).wrapping_add(__sn_numeric_rhs) as i64; let __sn_place_raw_index_0 = index(&mut (calls), 1); let __sn_place_index_0 = if __sn_place_raw_index_0 < 0 { __sn_place_raw_index_0 + (rows).len() as i64 } else { __sn_place_raw_index_0 }; let __sn_place_raw_index_1 = index(&mut (calls), 0); let __sn_place_index_1 = if __sn_place_raw_index_1 < 0 { let __sn_place_raw_index_2 = index(&mut (calls), 1); let __sn_place_index_2 = if __sn_place_raw_index_2 < 0 { __sn_place_raw_index_2 + (rows).len() as i64 } else { __sn_place_raw_index_2 }; __sn_place_raw_index_1 + ((rows)[__sn_place_index_2 as usize]).len() as i64 } else { __sn_place_raw_index_1 }; let __sn_numeric_place: &mut i64 = &mut (((rows)[__sn_place_index_0 as usize])[__sn_place_index_1 as usize]); *__sn_numeric_place = __sn_numeric_next; __sn_numeric_next });
    println!("{}", calls);
    println!("{}", ((rows)[__sn_index((rows).len(), 1)])[__sn_index(((rows)[__sn_index((rows).len(), 1)]).len(), 0)]);
    (calls = 0);
    println!("{}", { let __sn_numeric_old_1: i64 = { let __sn_place_raw_index_9 = index(&mut (calls), 1); let __sn_place_index_9 = if __sn_place_raw_index_9 < 0 { __sn_place_raw_index_9 + (rows).len() as i64 } else { __sn_place_raw_index_9 }; let __sn_place_raw_index_10 = index(&mut (calls), (-1)); let __sn_place_index_10 = if __sn_place_raw_index_10 < 0 { let __sn_place_raw_index_11 = index(&mut (calls), 1); let __sn_place_index_11 = if __sn_place_raw_index_11 < 0 { __sn_place_raw_index_11 + (rows).len() as i64 } else { __sn_place_raw_index_11 }; __sn_place_raw_index_10 + ((rows)[__sn_place_index_11 as usize]).len() as i64 } else { __sn_place_raw_index_10 }; ((rows)[__sn_place_index_9 as usize])[__sn_place_index_10 as usize] }; let __sn_numeric_rhs_1: i64 = (((3i128) as i64)) as i64; let __sn_numeric_next_1: i64 = (__sn_numeric_old_1 as i64).wrapping_sub(__sn_numeric_rhs_1) as i64; let __sn_place_raw_index_6 = index(&mut (calls), 1); let __sn_place_index_6 = if __sn_place_raw_index_6 < 0 { __sn_place_raw_index_6 + (rows).len() as i64 } else { __sn_place_raw_index_6 }; let __sn_place_raw_index_7 = index(&mut (calls), (-1)); let __sn_place_index_7 = if __sn_place_raw_index_7 < 0 { let __sn_place_raw_index_8 = index(&mut (calls), 1); let __sn_place_index_8 = if __sn_place_raw_index_8 < 0 { __sn_place_raw_index_8 + (rows).len() as i64 } else { __sn_place_raw_index_8 }; __sn_place_raw_index_7 + ((rows)[__sn_place_index_8 as usize]).len() as i64 } else { __sn_place_raw_index_7 }; let __sn_numeric_place_1: &mut i64 = &mut (((rows)[__sn_place_index_6 as usize])[__sn_place_index_7 as usize]); *__sn_numeric_place_1 = __sn_numeric_next_1; __sn_numeric_next_1 });
    println!("{}", calls);
    println!("{}", ((rows)[__sn_index((rows).len(), 1)])[__sn_index(((rows)[__sn_index((rows).len(), 1)]).len(), 1)]);
    (calls = 0);
    println!("{}", { let __sn_place_raw_index_12 = index(&mut (calls), (-1)); let __sn_place_index_12 = if __sn_place_raw_index_12 < 0 { __sn_place_raw_index_12 + (rows).len() as i64 } else { __sn_place_raw_index_12 }; let __sn_place_raw_index_13 = index(&mut (calls), (-1)); let __sn_place_index_13 = if __sn_place_raw_index_13 < 0 { let __sn_place_raw_index_14 = index(&mut (calls), (-1)); let __sn_place_index_14 = if __sn_place_raw_index_14 < 0 { __sn_place_raw_index_14 + (rows).len() as i64 } else { __sn_place_raw_index_14 }; __sn_place_raw_index_13 + ((rows)[__sn_place_index_14 as usize]).len() as i64 } else { __sn_place_raw_index_13 }; let __sn_numeric_place_2: &mut i64 = &mut (((rows)[__sn_place_index_12 as usize])[__sn_place_index_13 as usize]); let __sn_numeric_old_2 = *__sn_numeric_place_2; *__sn_numeric_place_2 = (__sn_numeric_old_2 as i64).wrapping_add(1) as i64; __sn_numeric_old_2 });
    println!("{}", calls);
    println!("{}", ((rows)[__sn_index((rows).len(), 1)])[__sn_index(((rows)[__sn_index((rows).len(), 1)]).len(), 1)]);
    let mut outers: Vec<Outer> = vec![Outer { inner: Inner { value: 7 } }];
    (calls = 0);
    println!("{}", { let __sn_numeric_old_3: i64 = { let __sn_place_raw_index_16 = index(&mut (calls), 0); let __sn_place_index_16 = if __sn_place_raw_index_16 < 0 { __sn_place_raw_index_16 + (outers).len() as i64 } else { __sn_place_raw_index_16 }; (((outers)[__sn_place_index_16 as usize]).inner).value }; let __sn_numeric_rhs_3: i64 = (((2i128) as i64)) as i64; let __sn_numeric_next_3: i64 = (__sn_numeric_old_3 as i64).wrapping_add(__sn_numeric_rhs_3) as i64; let __sn_place_raw_index_15 = index(&mut (calls), 0); let __sn_place_index_15 = if __sn_place_raw_index_15 < 0 { __sn_place_raw_index_15 + (outers).len() as i64 } else { __sn_place_raw_index_15 }; let __sn_numeric_place_3: &mut i64 = &mut ((((outers)[__sn_place_index_15 as usize]).inner).value); *__sn_numeric_place_3 = __sn_numeric_next_3; __sn_numeric_next_3 });
    println!("{}", calls);
    println!("{}", { let __sn_place_raw_index_17 = 0; let __sn_place_index_17 = if __sn_place_raw_index_17 < 0 { __sn_place_raw_index_17 + (outers).len() as i64 } else { __sn_place_raw_index_17 }; let __sn_numeric_place_4: &mut i64 = &mut ((((outers)[__sn_place_index_17 as usize]).inner).value); let __sn_numeric_old_4 = *__sn_numeric_place_4; *__sn_numeric_place_4 = (__sn_numeric_old_4 as i64).wrapping_sub(1) as i64; __sn_numeric_old_4 });
    println!("{}", (((outers)[__sn_index((outers).len(), 0)]).inner).value);
}
