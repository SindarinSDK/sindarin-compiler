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

#[derive(Clone, Copy, Debug, PartialEq)]
struct Inner {
    value: i64,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Outer {
    inner: Inner,
}

fn mutate(values: &mut Vec<i64>) -> i64 {
    { let __sn_numeric_old: i64 = { let __sn_place_raw_index_1 = 0; let __sn_place_index_1 = if __sn_place_raw_index_1 < 0 { __sn_place_raw_index_1 + (values).len() as i64 } else { __sn_place_raw_index_1 }; (values)[__sn_place_index_1 as usize] }; let __sn_numeric_rhs: i64 = (((3i128) as i64)) as i64; let __sn_numeric_next: i64 = (__sn_numeric_old as i64).wrapping_add(__sn_numeric_rhs) as i64; let __sn_place_raw_index_0 = 0; let __sn_place_index_0 = if __sn_place_raw_index_0 < 0 { __sn_place_raw_index_0 + (values).len() as i64 } else { __sn_place_raw_index_0 }; let __sn_numeric_place: &mut i64 = &mut ((values)[__sn_place_index_0 as usize]); *__sn_numeric_place = __sn_numeric_next; __sn_numeric_next };
    return { let __sn_place_raw_index_2 = 0; let __sn_place_index_2 = if __sn_place_raw_index_2 < 0 { __sn_place_raw_index_2 + (values).len() as i64 } else { __sn_place_raw_index_2 }; let __sn_numeric_place_1: &mut i64 = &mut ((values)[__sn_place_index_2 as usize]); let __sn_numeric_old_1 = *__sn_numeric_place_1; *__sn_numeric_place_1 = (__sn_numeric_old_1 as i64).wrapping_add(1) as i64; __sn_numeric_old_1 };
}

fn mutateLast(values: &mut Vec<i64>) -> i64 {
    return { let __sn_numeric_old_2: i64 = { let __sn_place_raw_index_4 = (-1); let __sn_place_index_4 = if __sn_place_raw_index_4 < 0 { __sn_place_raw_index_4 + (values).len() as i64 } else { __sn_place_raw_index_4 }; (values)[__sn_place_index_4 as usize] }; let __sn_numeric_rhs_2: i64 = (((2i128) as i64)) as i64; let __sn_numeric_next_2: i64 = (__sn_numeric_old_2 as i64).wrapping_sub(__sn_numeric_rhs_2) as i64; let __sn_place_raw_index_3 = (-1); let __sn_place_index_3 = if __sn_place_raw_index_3 < 0 { __sn_place_raw_index_3 + (values).len() as i64 } else { __sn_place_raw_index_3 }; let __sn_numeric_place_2: &mut i64 = &mut ((values)[__sn_place_index_3 as usize]); *__sn_numeric_place_2 = __sn_numeric_next_2; __sn_numeric_next_2 };
}

fn mutateCopy(mut outer: Outer) -> i64 {
    { let __sn_numeric_place_3: &mut i64 = &mut (((outer).inner).value); let __sn_numeric_old_3 = *__sn_numeric_place_3; *__sn_numeric_place_3 = (__sn_numeric_old_3 as i64).wrapping_add(1) as i64; __sn_numeric_old_3 };
    return { let __sn_numeric_old_4: i64 = { ((outer).inner).value }; let __sn_numeric_rhs_4: i64 = (((2i128) as i64)) as i64; let __sn_numeric_next_4: i64 = (__sn_numeric_old_4 as i64).wrapping_add(__sn_numeric_rhs_4) as i64; let __sn_numeric_place_4: &mut i64 = &mut (((outer).inner).value); *__sn_numeric_place_4 = __sn_numeric_next_4; __sn_numeric_next_4 };
}

fn mutateOuter(outer: &mut Outer) -> i64 {
    return { let __sn_numeric_place_5: &mut i64 = &mut (((outer).inner).value); let __sn_numeric_old_5 = *__sn_numeric_place_5; *__sn_numeric_place_5 = (__sn_numeric_old_5 as i64).wrapping_sub(1) as i64; __sn_numeric_old_5 };
}

fn same(first: &mut Vec<i64>, second: &mut Vec<i64>) -> i64 {
    { let __sn_numeric_old_6: i64 = { let __sn_place_raw_index_6 = 0; let __sn_place_index_6 = if __sn_place_raw_index_6 < 0 { __sn_place_raw_index_6 + (first).len() as i64 } else { __sn_place_raw_index_6 }; (first)[__sn_place_index_6 as usize] }; let __sn_numeric_rhs_6: i64 = (((2i128) as i64)) as i64; let __sn_numeric_next_6: i64 = (__sn_numeric_old_6 as i64).wrapping_add(__sn_numeric_rhs_6) as i64; let __sn_place_raw_index_5 = 0; let __sn_place_index_5 = if __sn_place_raw_index_5 < 0 { __sn_place_raw_index_5 + (first).len() as i64 } else { __sn_place_raw_index_5 }; let __sn_numeric_place_6: &mut i64 = &mut ((first)[__sn_place_index_5 as usize]); *__sn_numeric_place_6 = __sn_numeric_next_6; __sn_numeric_next_6 };
    { let __sn_numeric_old_7: i64 = { let __sn_place_raw_index_8 = 0; let __sn_place_index_8 = if __sn_place_raw_index_8 < 0 { __sn_place_raw_index_8 + (second).len() as i64 } else { __sn_place_raw_index_8 }; (second)[__sn_place_index_8 as usize] }; let __sn_numeric_rhs_7: i64 = (((3i128) as i64)) as i64; let __sn_numeric_next_7: i64 = (__sn_numeric_old_7 as i64).wrapping_mul(__sn_numeric_rhs_7) as i64; let __sn_place_raw_index_7 = 0; let __sn_place_index_7 = if __sn_place_raw_index_7 < 0 { __sn_place_raw_index_7 + (second).len() as i64 } else { __sn_place_raw_index_7 }; let __sn_numeric_place_7: &mut i64 = &mut ((second)[__sn_place_index_7 as usize]); *__sn_numeric_place_7 = __sn_numeric_next_7; __sn_numeric_next_7 };
    return (first)[__sn_index((first).len(), 0)];
}

fn main() {
    let mut values: Vec<i64> = vec![4, 8];
    println!("{}", mutate(&mut (values)));
    println!("{}", (values)[__sn_index((values).len(), 0)]);
    println!("{}", mutateLast(&mut (values)));
    println!("{}", (values)[__sn_index((values).len(), 1)]);
    let mut outer: Outer = Outer { inner: Inner { value: 10 } };
    println!("{}", mutateCopy(outer));
    println!("{}", ((outer).inner).value);
    println!("{}", mutateOuter(&mut (outer)));
    println!("{}", ((outer).inner).value);
    println!("{}", __sn_array_alias_call_0(&mut (values)));
    println!("{}", (values)[__sn_index((values).len(), 0)]);
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout;
        crate::__sn_stdio_fflush(stream);
    }
}

fn __sn_array_alias_call_0(first: &mut Vec<i64>) -> i64 {
    { let __sn_numeric_old_8: i64 = { let __sn_place_raw_index_10 = 0; let __sn_place_index_10 = if __sn_place_raw_index_10 < 0 { __sn_place_raw_index_10 + (first).len() as i64 } else { __sn_place_raw_index_10 }; (first)[__sn_place_index_10 as usize] }; let __sn_numeric_rhs_8: i64 = (((2i128) as i64)) as i64; let __sn_numeric_next_8: i64 = (__sn_numeric_old_8 as i64).wrapping_add(__sn_numeric_rhs_8) as i64; let __sn_place_raw_index_9 = 0; let __sn_place_index_9 = if __sn_place_raw_index_9 < 0 { __sn_place_raw_index_9 + (first).len() as i64 } else { __sn_place_raw_index_9 }; let __sn_numeric_place_8: &mut i64 = &mut ((first)[__sn_place_index_9 as usize]); *__sn_numeric_place_8 = __sn_numeric_next_8; __sn_numeric_next_8 };
    { let __sn_numeric_old_9: i64 = { let __sn_place_raw_index_12 = 0; let __sn_place_index_12 = if __sn_place_raw_index_12 < 0 { __sn_place_raw_index_12 + (first).len() as i64 } else { __sn_place_raw_index_12 }; (first)[__sn_place_index_12 as usize] }; let __sn_numeric_rhs_9: i64 = (((3i128) as i64)) as i64; let __sn_numeric_next_9: i64 = (__sn_numeric_old_9 as i64).wrapping_mul(__sn_numeric_rhs_9) as i64; let __sn_place_raw_index_11 = 0; let __sn_place_index_11 = if __sn_place_raw_index_11 < 0 { __sn_place_raw_index_11 + (first).len() as i64 } else { __sn_place_raw_index_11 }; let __sn_numeric_place_9: &mut i64 = &mut ((first)[__sn_place_index_11 as usize]); *__sn_numeric_place_9 = __sn_numeric_next_9; __sn_numeric_next_9 };
    return (first)[__sn_index((first).len(), 0)];
}
