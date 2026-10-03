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
    let mut values: Vec<i64> = vec![10, 21, 16, 8, 17];
    println!("{}", { let __sn_numeric_old: i64 = { let __sn_place_raw_index_1 = 0; let __sn_place_index_1 = if __sn_place_raw_index_1 < 0 { __sn_place_raw_index_1 + (values).len() as i64 } else { __sn_place_raw_index_1 }; (values)[__sn_place_index_1 as usize] }; let __sn_numeric_rhs: i64 = (((5i128) as i64)) as i64; let __sn_numeric_next: i64 = (__sn_numeric_old as i64).wrapping_add(__sn_numeric_rhs) as i64; let __sn_place_raw_index_0 = 0; let __sn_place_index_0 = if __sn_place_raw_index_0 < 0 { __sn_place_raw_index_0 + (values).len() as i64 } else { __sn_place_raw_index_0 }; let __sn_numeric_place: &mut i64 = &mut ((values)[__sn_place_index_0 as usize]); *__sn_numeric_place = __sn_numeric_next; __sn_numeric_next });
    println!("{}", { let __sn_numeric_old_1: i64 = { let __sn_place_raw_index_3 = 1; let __sn_place_index_3 = if __sn_place_raw_index_3 < 0 { __sn_place_raw_index_3 + (values).len() as i64 } else { __sn_place_raw_index_3 }; (values)[__sn_place_index_3 as usize] }; let __sn_numeric_rhs_1: i64 = (((4i128) as i64)) as i64; let __sn_numeric_next_1: i64 = (__sn_numeric_old_1 as i64).wrapping_sub(__sn_numeric_rhs_1) as i64; let __sn_place_raw_index_2 = 1; let __sn_place_index_2 = if __sn_place_raw_index_2 < 0 { __sn_place_raw_index_2 + (values).len() as i64 } else { __sn_place_raw_index_2 }; let __sn_numeric_place_1: &mut i64 = &mut ((values)[__sn_place_index_2 as usize]); *__sn_numeric_place_1 = __sn_numeric_next_1; __sn_numeric_next_1 });
    println!("{}", { let __sn_numeric_old_2: i64 = { let __sn_place_raw_index_5 = 2; let __sn_place_index_5 = if __sn_place_raw_index_5 < 0 { __sn_place_raw_index_5 + (values).len() as i64 } else { __sn_place_raw_index_5 }; (values)[__sn_place_index_5 as usize] }; let __sn_numeric_rhs_2: i64 = (((3i128) as i64)) as i64; let __sn_numeric_next_2: i64 = (__sn_numeric_old_2 as i64).wrapping_mul(__sn_numeric_rhs_2) as i64; let __sn_place_raw_index_4 = 2; let __sn_place_index_4 = if __sn_place_raw_index_4 < 0 { __sn_place_raw_index_4 + (values).len() as i64 } else { __sn_place_raw_index_4 }; let __sn_numeric_place_2: &mut i64 = &mut ((values)[__sn_place_index_4 as usize]); *__sn_numeric_place_2 = __sn_numeric_next_2; __sn_numeric_next_2 });
    println!("{}", { let __sn_numeric_old_3: i64 = { let __sn_place_raw_index_7 = 3; let __sn_place_index_7 = if __sn_place_raw_index_7 < 0 { __sn_place_raw_index_7 + (values).len() as i64 } else { __sn_place_raw_index_7 }; (values)[__sn_place_index_7 as usize] }; let __sn_numeric_rhs_3: i64 = (((2i128) as i64)) as i64; let __sn_numeric_next_3: i64 = ((__sn_numeric_old_3 as i64) / __sn_numeric_rhs_3) as i64; let __sn_place_raw_index_6 = 3; let __sn_place_index_6 = if __sn_place_raw_index_6 < 0 { __sn_place_raw_index_6 + (values).len() as i64 } else { __sn_place_raw_index_6 }; let __sn_numeric_place_3: &mut i64 = &mut ((values)[__sn_place_index_6 as usize]); *__sn_numeric_place_3 = __sn_numeric_next_3; __sn_numeric_next_3 });
    println!("{}", { let __sn_numeric_old_4: i64 = { let __sn_place_raw_index_9 = 4; let __sn_place_index_9 = if __sn_place_raw_index_9 < 0 { __sn_place_raw_index_9 + (values).len() as i64 } else { __sn_place_raw_index_9 }; (values)[__sn_place_index_9 as usize] }; let __sn_numeric_rhs_4: i64 = (((5i128) as i64)) as i64; let __sn_numeric_next_4: i64 = ((__sn_numeric_old_4 as i64) % __sn_numeric_rhs_4) as i64; let __sn_place_raw_index_8 = 4; let __sn_place_index_8 = if __sn_place_raw_index_8 < 0 { __sn_place_raw_index_8 + (values).len() as i64 } else { __sn_place_raw_index_8 }; let __sn_numeric_place_4: &mut i64 = &mut ((values)[__sn_place_index_8 as usize]); *__sn_numeric_place_4 = __sn_numeric_next_4; __sn_numeric_next_4 });
    println!("{}", { let __sn_place_raw_index_10 = 0; let __sn_place_index_10 = if __sn_place_raw_index_10 < 0 { __sn_place_raw_index_10 + (values).len() as i64 } else { __sn_place_raw_index_10 }; let __sn_numeric_place_5: &mut i64 = &mut ((values)[__sn_place_index_10 as usize]); let __sn_numeric_old_5 = *__sn_numeric_place_5; *__sn_numeric_place_5 = (__sn_numeric_old_5 as i64).wrapping_add(1) as i64; __sn_numeric_old_5 });
    println!("{}", { let __sn_place_raw_index_11 = 0; let __sn_place_index_11 = if __sn_place_raw_index_11 < 0 { __sn_place_raw_index_11 + (values).len() as i64 } else { __sn_place_raw_index_11 }; let __sn_numeric_place_6: &mut i64 = &mut ((values)[__sn_place_index_11 as usize]); let __sn_numeric_old_6 = *__sn_numeric_place_6; *__sn_numeric_place_6 = (__sn_numeric_old_6 as i64).wrapping_sub(1) as i64; __sn_numeric_old_6 });
    println!("{}", (values)[__sn_index((values).len(), 0)]);
    let mut bytes: Vec<u8> = vec![250];
    println!("0x{:02X}", ({ let __sn_numeric_old_7: u8 = { let __sn_place_raw_index_13 = 0; let __sn_place_index_13 = if __sn_place_raw_index_13 < 0 { __sn_place_raw_index_13 + (bytes).len() as i64 } else { __sn_place_raw_index_13 }; (bytes)[__sn_place_index_13 as usize] }; let __sn_numeric_rhs_7: i32 = (10) as i32; let __sn_numeric_next_7: u8 = (__sn_numeric_old_7 as i32).wrapping_add(__sn_numeric_rhs_7) as u8; let __sn_place_raw_index_12 = 0; let __sn_place_index_12 = if __sn_place_raw_index_12 < 0 { __sn_place_raw_index_12 + (bytes).len() as i64 } else { __sn_place_raw_index_12 }; let __sn_numeric_place_7: &mut u8 = &mut ((bytes)[__sn_place_index_12 as usize]); *__sn_numeric_place_7 = __sn_numeric_next_7; __sn_numeric_next_7 } as u32));
    println!("0x{:02X}", ({ let __sn_numeric_old_8: u8 = { let __sn_place_raw_index_15 = 0; let __sn_place_index_15 = if __sn_place_raw_index_15 < 0 { __sn_place_raw_index_15 + (bytes).len() as i64 } else { __sn_place_raw_index_15 }; (bytes)[__sn_place_index_15 as usize] }; let __sn_numeric_rhs_8: i32 = (100) as i32; let __sn_numeric_next_8: u8 = (__sn_numeric_old_8 as i32).wrapping_mul(__sn_numeric_rhs_8) as u8; let __sn_place_raw_index_14 = 0; let __sn_place_index_14 = if __sn_place_raw_index_14 < 0 { __sn_place_raw_index_14 + (bytes).len() as i64 } else { __sn_place_raw_index_14 }; let __sn_numeric_place_8: &mut u8 = &mut ((bytes)[__sn_place_index_14 as usize]); *__sn_numeric_place_8 = __sn_numeric_next_8; __sn_numeric_next_8 } as u32));
    println!("0x{:02X}", ({ let __sn_numeric_old_9: u8 = { let __sn_place_raw_index_17 = 0; let __sn_place_index_17 = if __sn_place_raw_index_17 < 0 { __sn_place_raw_index_17 + (bytes).len() as i64 } else { __sn_place_raw_index_17 }; (bytes)[__sn_place_index_17 as usize] }; let __sn_numeric_rhs_9: i32 = (3) as i32; let __sn_numeric_next_9: u8 = ((__sn_numeric_old_9 as i32) / __sn_numeric_rhs_9) as u8; let __sn_place_raw_index_16 = 0; let __sn_place_index_16 = if __sn_place_raw_index_16 < 0 { __sn_place_raw_index_16 + (bytes).len() as i64 } else { __sn_place_raw_index_16 }; let __sn_numeric_place_9: &mut u8 = &mut ((bytes)[__sn_place_index_16 as usize]); *__sn_numeric_place_9 = __sn_numeric_next_9; __sn_numeric_next_9 } as u32));
    println!("0x{:02X}", ({ let __sn_numeric_old_10: u8 = { let __sn_place_raw_index_19 = 0; let __sn_place_index_19 = if __sn_place_raw_index_19 < 0 { __sn_place_raw_index_19 + (bytes).len() as i64 } else { __sn_place_raw_index_19 }; (bytes)[__sn_place_index_19 as usize] }; let __sn_numeric_rhs_10: i32 = (5) as i32; let __sn_numeric_next_10: u8 = ((__sn_numeric_old_10 as i32) % __sn_numeric_rhs_10) as u8; let __sn_place_raw_index_18 = 0; let __sn_place_index_18 = if __sn_place_raw_index_18 < 0 { __sn_place_raw_index_18 + (bytes).len() as i64 } else { __sn_place_raw_index_18 }; let __sn_numeric_place_10: &mut u8 = &mut ((bytes)[__sn_place_index_18 as usize]); *__sn_numeric_place_10 = __sn_numeric_next_10; __sn_numeric_next_10 } as u32));
    println!("0x{:02X}", ({ let __sn_numeric_old_11: u8 = { let __sn_place_raw_index_21 = 0; let __sn_place_index_21 = if __sn_place_raw_index_21 < 0 { __sn_place_raw_index_21 + (bytes).len() as i64 } else { __sn_place_raw_index_21 }; (bytes)[__sn_place_index_21 as usize] }; let __sn_numeric_rhs_11: i32 = (1) as i32; let __sn_numeric_next_11: u8 = (__sn_numeric_old_11 as i32).wrapping_shl(__sn_numeric_rhs_11 as u32) as u8; let __sn_place_raw_index_20 = 0; let __sn_place_index_20 = if __sn_place_raw_index_20 < 0 { __sn_place_raw_index_20 + (bytes).len() as i64 } else { __sn_place_raw_index_20 }; let __sn_numeric_place_11: &mut u8 = &mut ((bytes)[__sn_place_index_20 as usize]); *__sn_numeric_place_11 = __sn_numeric_next_11; __sn_numeric_next_11 } as u32));
    println!("0x{:02X}", ({ let __sn_numeric_old_12: u8 = { let __sn_place_raw_index_23 = 0; let __sn_place_index_23 = if __sn_place_raw_index_23 < 0 { __sn_place_raw_index_23 + (bytes).len() as i64 } else { __sn_place_raw_index_23 }; (bytes)[__sn_place_index_23 as usize] }; let __sn_numeric_rhs_12: i32 = (1) as i32; let __sn_numeric_next_12: u8 = (__sn_numeric_old_12 as i32).wrapping_shr(__sn_numeric_rhs_12 as u32) as u8; let __sn_place_raw_index_22 = 0; let __sn_place_index_22 = if __sn_place_raw_index_22 < 0 { __sn_place_raw_index_22 + (bytes).len() as i64 } else { __sn_place_raw_index_22 }; let __sn_numeric_place_12: &mut u8 = &mut ((bytes)[__sn_place_index_22 as usize]); *__sn_numeric_place_12 = __sn_numeric_next_12; __sn_numeric_next_12 } as u32));
    println!("0x{:02X}", ({ let __sn_numeric_old_13: u8 = { let __sn_place_raw_index_25 = 0; let __sn_place_index_25 = if __sn_place_raw_index_25 < 0 { __sn_place_raw_index_25 + (bytes).len() as i64 } else { __sn_place_raw_index_25 }; (bytes)[__sn_place_index_25 as usize] }; let __sn_numeric_rhs_13: i32 = (4) as i32; let __sn_numeric_next_13: u8 = ((__sn_numeric_old_13 as i32) ^ __sn_numeric_rhs_13) as u8; let __sn_place_raw_index_24 = 0; let __sn_place_index_24 = if __sn_place_raw_index_24 < 0 { __sn_place_raw_index_24 + (bytes).len() as i64 } else { __sn_place_raw_index_24 }; let __sn_numeric_place_13: &mut u8 = &mut ((bytes)[__sn_place_index_24 as usize]); *__sn_numeric_place_13 = __sn_numeric_next_13; __sn_numeric_next_13 } as u32));
    println!("0x{:02X}", ({ let __sn_numeric_old_14: u8 = { let __sn_place_raw_index_27 = 0; let __sn_place_index_27 = if __sn_place_raw_index_27 < 0 { __sn_place_raw_index_27 + (bytes).len() as i64 } else { __sn_place_raw_index_27 }; (bytes)[__sn_place_index_27 as usize] }; let __sn_numeric_rhs_14: i32 = (3) as i32; let __sn_numeric_next_14: u8 = ((__sn_numeric_old_14 as i32) & __sn_numeric_rhs_14) as u8; let __sn_place_raw_index_26 = 0; let __sn_place_index_26 = if __sn_place_raw_index_26 < 0 { __sn_place_raw_index_26 + (bytes).len() as i64 } else { __sn_place_raw_index_26 }; let __sn_numeric_place_14: &mut u8 = &mut ((bytes)[__sn_place_index_26 as usize]); *__sn_numeric_place_14 = __sn_numeric_next_14; __sn_numeric_next_14 } as u32));
    println!("0x{:02X}", ({ let __sn_numeric_old_15: u8 = { let __sn_place_raw_index_29 = 0; let __sn_place_index_29 = if __sn_place_raw_index_29 < 0 { __sn_place_raw_index_29 + (bytes).len() as i64 } else { __sn_place_raw_index_29 }; (bytes)[__sn_place_index_29 as usize] }; let __sn_numeric_rhs_15: i32 = (8) as i32; let __sn_numeric_next_15: u8 = ((__sn_numeric_old_15 as i32) | __sn_numeric_rhs_15) as u8; let __sn_place_raw_index_28 = 0; let __sn_place_index_28 = if __sn_place_raw_index_28 < 0 { __sn_place_raw_index_28 + (bytes).len() as i64 } else { __sn_place_raw_index_28 }; let __sn_numeric_place_15: &mut u8 = &mut ((bytes)[__sn_place_index_28 as usize]); *__sn_numeric_place_15 = __sn_numeric_next_15; __sn_numeric_next_15 } as u32));
    let mut narrow: Vec<i32> = vec![20];
    let mut wideRhs: i64 = 3;
    println!("{}", { let __sn_numeric_old_16: i32 = { let __sn_place_raw_index_31 = 0; let __sn_place_index_31 = if __sn_place_raw_index_31 < 0 { __sn_place_raw_index_31 + (narrow).len() as i64 } else { __sn_place_raw_index_31 }; (narrow)[__sn_place_index_31 as usize] }; let __sn_numeric_rhs_16: i64 = (wideRhs) as i64; let __sn_numeric_next_16: i32 = (__sn_numeric_old_16 as i64).wrapping_sub(__sn_numeric_rhs_16) as i32; let __sn_place_raw_index_30 = 0; let __sn_place_index_30 = if __sn_place_raw_index_30 < 0 { __sn_place_raw_index_30 + (narrow).len() as i64 } else { __sn_place_raw_index_30 }; let __sn_numeric_place_16: &mut i32 = &mut ((narrow)[__sn_place_index_30 as usize]); *__sn_numeric_place_16 = __sn_numeric_next_16; __sn_numeric_next_16 });
    let mut unsigned: Vec<u32> = vec![4294967295];
    println!("{}", { let __sn_numeric_old_17: u32 = { let __sn_place_raw_index_33 = 0; let __sn_place_index_33 = if __sn_place_raw_index_33 < 0 { __sn_place_raw_index_33 + (unsigned).len() as i64 } else { __sn_place_raw_index_33 }; (unsigned)[__sn_place_index_33 as usize] }; let __sn_numeric_rhs_17: i64 = (((1i128) as i64)) as i64; let __sn_numeric_next_17: u32 = (__sn_numeric_old_17 as i64).wrapping_add(__sn_numeric_rhs_17) as u32; let __sn_place_raw_index_32 = 0; let __sn_place_index_32 = if __sn_place_raw_index_32 < 0 { __sn_place_raw_index_32 + (unsigned).len() as i64 } else { __sn_place_raw_index_32 }; let __sn_numeric_place_17: &mut u32 = &mut ((unsigned)[__sn_place_index_32 as usize]); *__sn_numeric_place_17 = __sn_numeric_next_17; __sn_numeric_next_17 });
    println!("{}", { let __sn_place_raw_index_34 = 0; let __sn_place_index_34 = if __sn_place_raw_index_34 < 0 { __sn_place_raw_index_34 + (unsigned).len() as i64 } else { __sn_place_raw_index_34 }; let __sn_numeric_place_18: &mut u32 = &mut ((unsigned)[__sn_place_index_34 as usize]); let __sn_numeric_old_18 = *__sn_numeric_place_18; *__sn_numeric_place_18 = (__sn_numeric_old_18 as u32).wrapping_add(1) as u32; __sn_numeric_old_18 });
    println!("{}", { let __sn_place_raw_index_35 = 0; let __sn_place_index_35 = if __sn_place_raw_index_35 < 0 { __sn_place_raw_index_35 + (unsigned).len() as i64 } else { __sn_place_raw_index_35 }; let __sn_numeric_place_19: &mut u32 = &mut ((unsigned)[__sn_place_index_35 as usize]); let __sn_numeric_old_19 = *__sn_numeric_place_19; *__sn_numeric_place_19 = (__sn_numeric_old_19 as u32).wrapping_sub(1) as u32; __sn_numeric_old_19 });
    println!("{}", (unsigned)[__sn_index((unsigned).len(), 0)]);
    let mut wideUnsigned: Vec<u64> = vec![((-1i64) as u64)];
    let mut one: u64 = 1;
    println!("{}", ({ let __sn_numeric_old_20: u64 = { let __sn_place_raw_index_37 = 0; let __sn_place_index_37 = if __sn_place_raw_index_37 < 0 { __sn_place_raw_index_37 + (wideUnsigned).len() as i64 } else { __sn_place_raw_index_37 }; (wideUnsigned)[__sn_place_index_37 as usize] }; let __sn_numeric_rhs_20: u64 = (one) as u64; let __sn_numeric_next_20: u64 = (__sn_numeric_old_20 as u64).wrapping_add(__sn_numeric_rhs_20) as u64; let __sn_place_raw_index_36 = 0; let __sn_place_index_36 = if __sn_place_raw_index_36 < 0 { __sn_place_raw_index_36 + (wideUnsigned).len() as i64 } else { __sn_place_raw_index_36 }; let __sn_numeric_place_20: &mut u64 = &mut ((wideUnsigned)[__sn_place_index_36 as usize]); *__sn_numeric_place_20 = __sn_numeric_next_20; __sn_numeric_next_20 } as i64));
    println!("{}", ({ let __sn_place_raw_index_38 = 0; let __sn_place_index_38 = if __sn_place_raw_index_38 < 0 { __sn_place_raw_index_38 + (wideUnsigned).len() as i64 } else { __sn_place_raw_index_38 }; let __sn_numeric_place_21: &mut u64 = &mut ((wideUnsigned)[__sn_place_index_38 as usize]); let __sn_numeric_old_21 = *__sn_numeric_place_21; *__sn_numeric_place_21 = (__sn_numeric_old_21 as u64).wrapping_sub(1) as u64; __sn_numeric_old_21 } as i64));
    println!("{}", ((wideUnsigned)[__sn_index((wideUnsigned).len(), 0)] as i64));
    let mut longs: Vec<i64> = vec![30];
    println!("{}", { let __sn_numeric_old_22: i64 = { let __sn_place_raw_index_40 = 0; let __sn_place_index_40 = if __sn_place_raw_index_40 < 0 { __sn_place_raw_index_40 + (longs).len() as i64 } else { __sn_place_raw_index_40 }; (longs)[__sn_place_index_40 as usize] }; let __sn_numeric_rhs_22: i64 = (((3i128) as i64)) as i64; let __sn_numeric_next_22: i64 = ((__sn_numeric_old_22 as i64) / __sn_numeric_rhs_22) as i64; let __sn_place_raw_index_39 = 0; let __sn_place_index_39 = if __sn_place_raw_index_39 < 0 { __sn_place_raw_index_39 + (longs).len() as i64 } else { __sn_place_raw_index_39 }; let __sn_numeric_place_22: &mut i64 = &mut ((longs)[__sn_place_index_39 as usize]); *__sn_numeric_place_22 = __sn_numeric_next_22; __sn_numeric_next_22 });
    unsafe {
        #[cfg(windows)]
        let stream = crate::__sn_stdio_iob(1);
        #[cfg(not(windows))]
        let stream = crate::__sn_stdio_stdout;
        crate::__sn_stdio_fflush(stream);
    }
}
