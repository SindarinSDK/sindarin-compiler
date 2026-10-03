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
fn storedWide() -> f64 {
    return ((16777217.0) as f64);
}

fn asFloat(value: f32, calls: &mut i64) -> f32 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return value;
}

fn asDouble(value: f64, calls: &mut i64) -> f64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return value;
}

fn appendNeedle(values: &mut Vec<f32>, calls: &mut i64) -> f32 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_float_push_value = { let __sn_float_value_bytes = ((1.5f64)).to_ne_bytes(); let mut __sn_float_slot = [0u8; 4]; let __sn_float_count = std::cmp::min(__sn_float_slot.len(), __sn_float_value_bytes.len()); __sn_float_slot[..__sn_float_count].copy_from_slice(&__sn_float_value_bytes[..__sn_float_count]); f32::from_ne_bytes(__sn_float_slot) }; (values).push(__sn_float_push_value); };
    return 1.5;
}

fn main() {
    let __sn_stdio_guard = __SnStdioGuard;
    let mut calls: i64 = 0;
    let mut values: Vec<f32> = vec![0.0, 16777216.0];
    let mut wide: Vec<f64> = vec![16777217.0];
    println!("{}", { let __sn_array_search = (asFloat((((16777217.0f64)) as f32), &mut (calls))).to_ne_bytes(); let __sn_array = &(values); __sn_array.iter().position(|__sn_item| { let __sn_item_bytes = __sn_item.to_ne_bytes(); __sn_array_search.get(..__sn_item_bytes.len()) == Some(__sn_item_bytes.as_slice()) }).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    println!("{}", calls);
    println!("{}", { let __sn_array_search = (asDouble((16777217.0f64), &mut (calls))).to_ne_bytes(); let __sn_array = &(wide); __sn_array.iter().position(|__sn_item| { let __sn_item_bytes = __sn_item.to_ne_bytes(); __sn_array_search.get(..__sn_item_bytes.len()) == Some(__sn_item_bytes.as_slice()) }).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    println!("{}", calls);
    println!("{}", { let __sn_array_search = ((16777217.0f64)).to_ne_bytes(); let __sn_array = &(wide); __sn_array.iter().position(|__sn_item| { let __sn_item_bytes = __sn_item.to_ne_bytes(); __sn_array_search.get(..__sn_item_bytes.len()) == Some(__sn_item_bytes.as_slice()) }).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    println!("{}", { let __sn_array_search = ((-(16777217.0f64))).to_ne_bytes(); let __sn_array = &(wide); __sn_array.iter().position(|__sn_item| { let __sn_item_bytes = __sn_item.to_ne_bytes(); __sn_array_search.get(..__sn_item_bytes.len()) == Some(__sn_item_bytes.as_slice()) }).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    let mut empty: Vec<f64> = vec![];
    println!("{}", { let __sn_array_search = (asFloat((((16777217.0f64)) as f32), &mut (calls))).to_ne_bytes(); let __sn_array = &(empty); __sn_array.iter().any(|__sn_item| { let __sn_item_bytes = __sn_item.to_ne_bytes(); __sn_array_search.get(..__sn_item_bytes.len()) == Some(__sn_item_bytes.as_slice()) }) });
    println!("{}", calls);
    println!("{}", { let __sn_array_search = (asFloat((((16777217.0f64)) as f32), &mut (calls))).to_ne_bytes(); let __sn_array = &(empty); __sn_array.iter().position(|__sn_item| { let __sn_item_bytes = __sn_item.to_ne_bytes(); __sn_array_search.get(..__sn_item_bytes.len()) == Some(__sn_item_bytes.as_slice()) }).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    println!("{}", calls);
    let mut callback: __SnClosure<dyn Fn(f64) -> f64> = { self::__SnClosure::<dyn Fn(f64) -> f64>(std::rc::Rc::new(move |value: f64| -> f64 { value.clone()})) }
;
    println!("{}", { let __sn_array_search = (((callback.clone()).0)((16777217.0f64))).to_ne_bytes(); let __sn_array = &(wide); __sn_array.iter().position(|__sn_item| { let __sn_item_bytes = __sn_item.to_ne_bytes(); __sn_array_search.get(..__sn_item_bytes.len()) == Some(__sn_item_bytes.as_slice()) }).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    println!("{}", { let __sn_array_search = (storedWide()).to_ne_bytes(); let __sn_array = &(wide); __sn_array.iter().position(|__sn_item| { let __sn_item_bytes = __sn_item.to_ne_bytes(); __sn_array_search.get(..__sn_item_bytes.len()) == Some(__sn_item_bytes.as_slice()) }).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    let mut mutableValues: Vec<f32> = vec![0.0];
    println!("{}", { let __sn_array_search = (appendNeedle(&mut (mutableValues), &mut (calls))).to_ne_bytes(); let __sn_array = &(mutableValues); __sn_array.iter().any(|__sn_item| { let __sn_item_bytes = __sn_item.to_ne_bytes(); __sn_array_search.get(..__sn_item_bytes.len()) == Some(__sn_item_bytes.as_slice()) }) });
    println!("{}", calls);
    println!("{}", (mutableValues).len() as i64);
    println!("{}", ((mutableValues)[__sn_index((mutableValues).len(), 1)] == 0.0));
}
