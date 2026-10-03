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

fn narrow(value: f32) -> f32 {
    return value;
}

fn main() {
    let __sn_stdio_guard = __SnStdioGuard;
    let mut values: Vec<f32> = vec![1.0, 2.0];
    { let __sn_numeric_old: f32 = { let __sn_place_raw_index_1 = 0; let __sn_place_index_1 = if __sn_place_raw_index_1 < 0 { __sn_place_raw_index_1 + (values).len() as i64 } else { __sn_place_raw_index_1 }; (values)[__sn_place_index_1 as usize] }; let __sn_numeric_rhs: f64 = ((1.0000000600000001f64)) as f64; let __sn_numeric_next: f32 = ((__sn_numeric_old as f64) - __sn_numeric_rhs) as f32; let __sn_place_raw_index_0 = 0; let __sn_place_index_0 = if __sn_place_raw_index_0 < 0 { __sn_place_raw_index_0 + (values).len() as i64 } else { __sn_place_raw_index_0 }; let __sn_numeric_place: &mut f32 = &mut ((values)[__sn_place_index_0 as usize]); *__sn_numeric_place = __sn_numeric_next; __sn_numeric_next };
    println!("{}", (((((values)[__sn_index((values).len(), 0)]) as f32) > (((-7.0000000000000005e-08)) as f32)) && ((((values)[__sn_index((values).len(), 0)]) as f32) < (((-4.9999999999999998e-08)) as f32))));
    let mut wide: f64 = 16777217.0;
    { let __sn_numeric_old_1: f32 = { let __sn_place_raw_index_3 = 1; let __sn_place_index_3 = if __sn_place_raw_index_3 < 0 { __sn_place_raw_index_3 + (values).len() as i64 } else { __sn_place_raw_index_3 }; (values)[__sn_place_index_3 as usize] }; let __sn_numeric_rhs_1: f64 = (wide) as f64; let __sn_numeric_next_1: f32 = ((__sn_numeric_old_1 as f64) + __sn_numeric_rhs_1) as f32; let __sn_place_raw_index_2 = 1; let __sn_place_index_2 = if __sn_place_raw_index_2 < 0 { __sn_place_raw_index_2 + (values).len() as i64 } else { __sn_place_raw_index_2 }; let __sn_numeric_place_1: &mut f32 = &mut ((values)[__sn_place_index_2 as usize]); *__sn_numeric_place_1 = __sn_numeric_next_1; __sn_numeric_next_1 };
    println!("{}", ((values)[__sn_index((values).len(), 1)] == 16777220.0));
    { let __sn_array_index = __sn_index((values).len(), 0); (values)[__sn_array_index] = 1.0; };
    { let __sn_numeric_old_2: f32 = { let __sn_place_raw_index_5 = 0; let __sn_place_index_5 = if __sn_place_raw_index_5 < 0 { __sn_place_raw_index_5 + (values).len() as i64 } else { __sn_place_raw_index_5 }; (values)[__sn_place_index_5 as usize] }; let __sn_numeric_rhs_2: f32 = (narrow((((1.0000000600000001f64)) as f32))) as f32; let __sn_numeric_next_2: f32 = ((__sn_numeric_old_2 as f32) - __sn_numeric_rhs_2) as f32; let __sn_place_raw_index_4 = 0; let __sn_place_index_4 = if __sn_place_raw_index_4 < 0 { __sn_place_raw_index_4 + (values).len() as i64 } else { __sn_place_raw_index_4 }; let __sn_numeric_place_2: &mut f32 = &mut ((values)[__sn_place_index_4 as usize]); *__sn_numeric_place_2 = __sn_numeric_next_2; __sn_numeric_next_2 };
    println!("{}", ((((values)[__sn_index((values).len(), 0)]) as f32) < (((-9.9999999999999995e-08)) as f32)));
    let mut doubles: Vec<f64> = vec![5.0];
    println!("{:.5}", { let __sn_numeric_old_3: f64 = { let __sn_place_raw_index_7 = 0; let __sn_place_index_7 = if __sn_place_raw_index_7 < 0 { __sn_place_raw_index_7 + (doubles).len() as i64 } else { __sn_place_raw_index_7 }; (doubles)[__sn_place_index_7 as usize] }; let __sn_numeric_rhs_3: f64 = ((0.5f64)) as f64; let __sn_numeric_next_3: f64 = ((__sn_numeric_old_3 as f64) + __sn_numeric_rhs_3) as f64; let __sn_place_raw_index_6 = 0; let __sn_place_index_6 = if __sn_place_raw_index_6 < 0 { __sn_place_raw_index_6 + (doubles).len() as i64 } else { __sn_place_raw_index_6 }; let __sn_numeric_place_3: &mut f64 = &mut ((doubles)[__sn_place_index_6 as usize]); *__sn_numeric_place_3 = __sn_numeric_next_3; __sn_numeric_next_3 });
    println!("{:.5}", { let __sn_numeric_old_4: f64 = { let __sn_place_raw_index_9 = 0; let __sn_place_index_9 = if __sn_place_raw_index_9 < 0 { __sn_place_raw_index_9 + (doubles).len() as i64 } else { __sn_place_raw_index_9 }; (doubles)[__sn_place_index_9 as usize] }; let __sn_numeric_rhs_4: f64 = ((1.0f64)) as f64; let __sn_numeric_next_4: f64 = ((__sn_numeric_old_4 as f64) - __sn_numeric_rhs_4) as f64; let __sn_place_raw_index_8 = 0; let __sn_place_index_8 = if __sn_place_raw_index_8 < 0 { __sn_place_raw_index_8 + (doubles).len() as i64 } else { __sn_place_raw_index_8 }; let __sn_numeric_place_4: &mut f64 = &mut ((doubles)[__sn_place_index_8 as usize]); *__sn_numeric_place_4 = __sn_numeric_next_4; __sn_numeric_next_4 });
    println!("{:.5}", { let __sn_numeric_old_5: f64 = { let __sn_place_raw_index_11 = 0; let __sn_place_index_11 = if __sn_place_raw_index_11 < 0 { __sn_place_raw_index_11 + (doubles).len() as i64 } else { __sn_place_raw_index_11 }; (doubles)[__sn_place_index_11 as usize] }; let __sn_numeric_rhs_5: f64 = ((2.0f64)) as f64; let __sn_numeric_next_5: f64 = ((__sn_numeric_old_5 as f64) * __sn_numeric_rhs_5) as f64; let __sn_place_raw_index_10 = 0; let __sn_place_index_10 = if __sn_place_raw_index_10 < 0 { __sn_place_raw_index_10 + (doubles).len() as i64 } else { __sn_place_raw_index_10 }; let __sn_numeric_place_5: &mut f64 = &mut ((doubles)[__sn_place_index_10 as usize]); *__sn_numeric_place_5 = __sn_numeric_next_5; __sn_numeric_next_5 });
    println!("{:.5}", { let __sn_numeric_old_6: f64 = { let __sn_place_raw_index_13 = 0; let __sn_place_index_13 = if __sn_place_raw_index_13 < 0 { __sn_place_raw_index_13 + (doubles).len() as i64 } else { __sn_place_raw_index_13 }; (doubles)[__sn_place_index_13 as usize] }; let __sn_numeric_rhs_6: f64 = ((3.0f64)) as f64; let __sn_numeric_next_6: f64 = ((__sn_numeric_old_6 as f64) / __sn_numeric_rhs_6) as f64; let __sn_place_raw_index_12 = 0; let __sn_place_index_12 = if __sn_place_raw_index_12 < 0 { __sn_place_raw_index_12 + (doubles).len() as i64 } else { __sn_place_raw_index_12 }; let __sn_numeric_place_6: &mut f64 = &mut ((doubles)[__sn_place_index_12 as usize]); *__sn_numeric_place_6 = __sn_numeric_next_6; __sn_numeric_next_6 });
    println!("{:.5}", { let __sn_place_raw_index_14 = 0; let __sn_place_index_14 = if __sn_place_raw_index_14 < 0 { __sn_place_raw_index_14 + (doubles).len() as i64 } else { __sn_place_raw_index_14 }; let __sn_numeric_place_7: &mut f64 = &mut ((doubles)[__sn_place_index_14 as usize]); let __sn_numeric_old_7 = *__sn_numeric_place_7; *__sn_numeric_place_7 = ((__sn_numeric_old_7 as f64) + 1.0) as f64; __sn_numeric_old_7 });
    println!("{:.5}", { let __sn_place_raw_index_15 = 0; let __sn_place_index_15 = if __sn_place_raw_index_15 < 0 { __sn_place_raw_index_15 + (doubles).len() as i64 } else { __sn_place_raw_index_15 }; let __sn_numeric_place_8: &mut f64 = &mut ((doubles)[__sn_place_index_15 as usize]); let __sn_numeric_old_8 = *__sn_numeric_place_8; *__sn_numeric_place_8 = ((__sn_numeric_old_8 as f64) - 1.0) as f64; __sn_numeric_old_8 });
    println!("{:.5}", (doubles)[__sn_index((doubles).len(), 0)]);
}
