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
    let __sn_stdio_guard = __SnStdioGuard;
    let mut values: Vec<i64> = vec![10];
    let mut action: __SnClosure<dyn Fn() -> i64> = { let (values, ) = (std::rc::Rc::new(std::cell::RefCell::new(values.clone())), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { { let __sn_numeric_old: i64 = { let __sn_place_raw_index_1 = 0; let __sn_place_index_1 = if __sn_place_raw_index_1 < 0 { __sn_place_raw_index_1 + (values.borrow().clone()).len() as i64 } else { __sn_place_raw_index_1 }; (values.borrow().clone())[__sn_place_index_1 as usize] }; let __sn_numeric_rhs: i64 = (((2i128) as i64)) as i64; let __sn_numeric_next: i64 = (__sn_numeric_old as i64).wrapping_add(__sn_numeric_rhs) as i64; let __sn_place_raw_index_0 = 0; let __sn_place_index_0 = if __sn_place_raw_index_0 < 0 { __sn_place_raw_index_0 + (values.borrow().clone()).len() as i64 } else { __sn_place_raw_index_0 }; let __sn_numeric_place: &mut i64 = &mut ((values.borrow_mut())[__sn_place_index_0 as usize]); *__sn_numeric_place = __sn_numeric_next; __sn_numeric_next };return { let __sn_place_raw_index_2 = 0; let __sn_place_index_2 = if __sn_place_raw_index_2 < 0 { __sn_place_raw_index_2 + (values.borrow().clone()).len() as i64 } else { __sn_place_raw_index_2 }; let __sn_numeric_place_1: &mut i64 = &mut ((values.borrow_mut())[__sn_place_index_2 as usize]); let __sn_numeric_old_1 = *__sn_numeric_place_1; *__sn_numeric_place_1 = (__sn_numeric_old_1 as i64).wrapping_add(1) as i64; __sn_numeric_old_1 };})) }
;
    println!("{}", ((action.clone()).0)());
    println!("{}", ((action.clone()).0)());
    println!("{}", (values)[__sn_index((values).len(), 0)]);
    let mut rows: Vec<Vec<f32>> = vec![vec![1.0]];
    let mut step: __SnClosure<dyn Fn() -> f32> = { let (rows, ) = (std::rc::Rc::new(std::cell::RefCell::new(rows.clone())), ); self::__SnClosure::<dyn Fn() -> f32>(std::rc::Rc::new(move || -> f32 { { let __sn_numeric_old_2: f32 = { let __sn_place_raw_index_6 = (-1); let __sn_place_index_6 = if __sn_place_raw_index_6 < 0 { __sn_place_raw_index_6 + (rows.borrow().clone()).len() as i64 } else { __sn_place_raw_index_6 }; let __sn_place_raw_index_7 = (-1); let __sn_place_index_7 = if __sn_place_raw_index_7 < 0 { let __sn_place_raw_index_8 = (-1); let __sn_place_index_8 = if __sn_place_raw_index_8 < 0 { __sn_place_raw_index_8 + (rows.borrow().clone()).len() as i64 } else { __sn_place_raw_index_8 }; __sn_place_raw_index_7 + ((rows.borrow().clone())[__sn_place_index_8 as usize]).len() as i64 } else { __sn_place_raw_index_7 }; ((rows.borrow().clone())[__sn_place_index_6 as usize])[__sn_place_index_7 as usize] }; let __sn_numeric_rhs_2: f64 = ((0.5f64)) as f64; let __sn_numeric_next_2: f32 = ((__sn_numeric_old_2 as f64) + __sn_numeric_rhs_2) as f32; let __sn_place_raw_index_3 = (-1); let __sn_place_index_3 = if __sn_place_raw_index_3 < 0 { __sn_place_raw_index_3 + (rows.borrow().clone()).len() as i64 } else { __sn_place_raw_index_3 }; let __sn_place_raw_index_4 = (-1); let __sn_place_index_4 = if __sn_place_raw_index_4 < 0 { let __sn_place_raw_index_5 = (-1); let __sn_place_index_5 = if __sn_place_raw_index_5 < 0 { __sn_place_raw_index_5 + (rows.borrow().clone()).len() as i64 } else { __sn_place_raw_index_5 }; __sn_place_raw_index_4 + ((rows.borrow().clone())[__sn_place_index_5 as usize]).len() as i64 } else { __sn_place_raw_index_4 }; let __sn_numeric_place_2: &mut f32 = &mut (((rows.borrow_mut())[__sn_place_index_3 as usize])[__sn_place_index_4 as usize]); *__sn_numeric_place_2 = __sn_numeric_next_2; __sn_numeric_next_2 };return { let __sn_place_raw_index_9 = 0; let __sn_place_index_9 = if __sn_place_raw_index_9 < 0 { __sn_place_raw_index_9 + (rows.borrow().clone()).len() as i64 } else { __sn_place_raw_index_9 }; let __sn_place_raw_index_10 = 0; let __sn_place_index_10 = if __sn_place_raw_index_10 < 0 { let __sn_place_raw_index_11 = 0; let __sn_place_index_11 = if __sn_place_raw_index_11 < 0 { __sn_place_raw_index_11 + (rows.borrow().clone()).len() as i64 } else { __sn_place_raw_index_11 }; __sn_place_raw_index_10 + ((rows.borrow().clone())[__sn_place_index_11 as usize]).len() as i64 } else { __sn_place_raw_index_10 }; let __sn_numeric_place_3: &mut f32 = &mut (((rows.borrow_mut())[__sn_place_index_9 as usize])[__sn_place_index_10 as usize]); let __sn_numeric_old_3 = *__sn_numeric_place_3; *__sn_numeric_place_3 = ((__sn_numeric_old_3 as f32) + 1.0) as f32; __sn_numeric_old_3 };})) }
;
    println!("{:.5}", ((step.clone()).0)());
    println!("{:.5}", ((step.clone()).0)());
    println!("{:.5}", ((rows)[__sn_index((rows).len(), 0)])[__sn_index(((rows)[__sn_index((rows).len(), 0)]).len(), 0)]);
}
