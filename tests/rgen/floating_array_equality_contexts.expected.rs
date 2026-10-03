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
#[derive(Clone, Debug, PartialEq)]
struct FloatingBag {
    values: Vec<f32>,
}

fn equalFloat(left: &mut Vec<f32>, right: &mut Vec<f32>) -> bool {
    return { let __sn_float_eq_right_2: &[f32] = &(right); let __sn_float_eq_left_2: &[f32] = &(left); (__sn_float_eq_left_2.len() == __sn_float_eq_right_2.len() && __sn_float_eq_left_2.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_2.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_2.len() * std::mem::size_of::<f32>()))) }
;
}

fn equalDouble(left: &mut Vec<f64>, right: &mut Vec<f64>) -> bool {
    return { let __sn_float_eq_right_3: &[f64] = &(right); let __sn_float_eq_left_3: &[f64] = &(left); (__sn_float_eq_left_3.len() == __sn_float_eq_right_3.len() && __sn_float_eq_left_3.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_3.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_3.len() * std::mem::size_of::<f64>()))) }
;
}

fn makeZero(calls: &mut i64) -> Vec<f32> {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return vec![0.0];
}

fn makeNegativeZero(calls: &mut i64) -> Vec<f32> {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return vec![(-0.0)];
}

fn orderedZero(calls: &mut i64, digit: i64) -> f32 {
    (*(calls) = __sn_checked_0((__sn_checked_0((*(calls)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(digit), "Runtime error: integer overflow in addition"));
    return 0.0;
}

fn main() {
    let __sn_stdio_guard = __SnStdioGuard;
    let mut positive: Vec<f32> = vec![0.0];
    let mut negative: Vec<f32> = vec![(-0.0)];
    println!("{}", equalFloat(&mut (positive), &mut (negative)));
    let mut positiveWide: Vec<f64> = vec![0.0];
    let mut negativeWide: Vec<f64> = vec![(-0.0)];
    println!("{}", equalDouble(&mut (positiveWide), &mut (negativeWide)));
    let mut calls: i64 = 0;
    println!("{}", { let __sn_float_eq_left_4: &[f32] = &(makeZero(&mut (calls))); let __sn_float_eq_right_4: &[f32] = &(negative); (__sn_float_eq_left_4.len() == __sn_float_eq_right_4.len() && __sn_float_eq_left_4.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_4.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_4.len() * std::mem::size_of::<f32>()))) }
);
    println!("{}", calls);
    println!("{}", { let __sn_float_eq_right_5: &[f32] = &(makeNegativeZero(&mut (calls))); let __sn_float_eq_left_5: &[f32] = &(positive); !(__sn_float_eq_left_5.len() == __sn_float_eq_right_5.len() && __sn_float_eq_left_5.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_5.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_5.len() * std::mem::size_of::<f32>()))) }
);
    println!("{}", calls);
    println!("{}", { let __sn_float_eq_left_6: &[f32] = &(makeZero(&mut (calls))); let __sn_float_eq_right_6: &[f32] = &(makeNegativeZero(&mut (calls))); (__sn_float_eq_left_6.len() == __sn_float_eq_right_6.len() && __sn_float_eq_left_6.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_6.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_6.len() * std::mem::size_of::<f32>()))) }
);
    println!("{}", calls);
    let mut compare: __SnClosure<dyn Fn() -> bool> = { let (positive, negative, ) = (positive.clone(), negative.clone(), ); self::__SnClosure::<dyn Fn() -> bool>(std::rc::Rc::new(move || -> bool { { let __sn_float_eq_right_7: &[f32] = &(negative.clone()); let __sn_float_eq_left_7: &[f32] = &(positive.clone()); (__sn_float_eq_left_7.len() == __sn_float_eq_right_7.len() && __sn_float_eq_left_7.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_7.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_7.len() * std::mem::size_of::<f32>()))) }
})) }
;
    println!("{}", ((compare.clone()).0)());
    let mut first: FloatingBag = FloatingBag { values: positive.clone() };
    let mut second: FloatingBag = FloatingBag { values: negative.clone() };
    println!("{}", { let __sn_float_eq_right_8: &[f32] = &((second).values); let __sn_float_eq_left_8: &[f32] = &((first).values); (__sn_float_eq_left_8.len() == __sn_float_eq_right_8.len() && __sn_float_eq_left_8.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_8.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_8.len() * std::mem::size_of::<f32>()))) }
);
    let mut nested: Vec<Vec<f32>> = vec![positive.clone(), negative.clone()];
    println!("{}", { let __sn_float_eq_left_9: &[f32] = &((nested)[__sn_index((nested).len(), 0)]); let __sn_float_eq_right_9: &[f32] = &((nested)[__sn_index((nested).len(), 1)]); (__sn_float_eq_left_9.len() == __sn_float_eq_right_9.len() && __sn_float_eq_left_9.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_9.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_9.len() * std::mem::size_of::<f32>()))) }
);
    let mut __sn_float_eq_left: Vec<f32> = vec![0.0];
    let mut __sn_float_eq_right: Vec<f32> = vec![(-0.0)];
    println!("{}", { let __sn_float_eq_right_10: &[f32] = &(__sn_float_eq_right); let __sn_float_eq_left_10: &[f32] = &(__sn_float_eq_left); (__sn_float_eq_left_10.len() == __sn_float_eq_right_10.len() && __sn_float_eq_left_10.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_10.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_10.len() * std::mem::size_of::<f32>()))) }
);
    (calls = 0);
    println!("{}", { let __sn_float_eq_right_11: &[f32] = &(vec![orderedZero(&mut (calls), 2)]); let __sn_float_eq_left_11: &[f32] = &(vec![orderedZero(&mut (calls), 1)]); (__sn_float_eq_left_11.len() == __sn_float_eq_right_11.len() && __sn_float_eq_left_11.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_11.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_11.len() * std::mem::size_of::<f32>()))) }
);
    println!("{}", calls);
    (calls = 0);
    println!("{}", { let __sn_float_eq_right_12: &[f32] = &(vec![orderedZero(&mut (calls), 2)]); let __sn_float_eq_left_12: &[f32] = &(vec![orderedZero(&mut (calls), 1)]); !(__sn_float_eq_left_12.len() == __sn_float_eq_right_12.len() && __sn_float_eq_left_12.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_12.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_12.len() * std::mem::size_of::<f32>()))) }
);
    println!("{}", calls);
    println!("{}", { let __sn_float_eq_right_13: &[f32] = &(vec![1.5]); let __sn_float_eq_left_13: &[f32] = &(vec![1.5]); (__sn_float_eq_left_13.len() == __sn_float_eq_right_13.len() && __sn_float_eq_left_13.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_13.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_13.len() * std::mem::size_of::<f32>()))) }
);
    println!("{}", { let __sn_float_eq_right_14: &[f32] = &(vec![(-0.0)]); let __sn_float_eq_left_14: &[f32] = &(vec![0.0]); (__sn_float_eq_left_14.len() == __sn_float_eq_right_14.len() && __sn_float_eq_left_14.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_14.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_14.len() * std::mem::size_of::<f32>()))) }
);
    println!("{}", { let __sn_float_eq_right_15: &[f32] = &(vec![1.5]); let __sn_float_eq_left_15: &[f32] = &(vec![1.5]); !(__sn_float_eq_left_15.len() == __sn_float_eq_right_15.len() && __sn_float_eq_left_15.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_15.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_15.len() * std::mem::size_of::<f32>()))) }
);
    println!("{}", { let __sn_float_eq_right_16: &[f64] = &(vec![1.5]); let __sn_float_eq_left_16: &[f64] = &(vec![1.5]); (__sn_float_eq_left_16.len() == __sn_float_eq_right_16.len() && __sn_float_eq_left_16.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_16.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_16.len() * std::mem::size_of::<f64>()))) }
);
    println!("{}", { let __sn_float_eq_right_17: &[f64] = &(vec![(-0.0)]); let __sn_float_eq_left_17: &[f64] = &(vec![0.0]); (__sn_float_eq_left_17.len() == __sn_float_eq_right_17.len() && __sn_float_eq_left_17.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_17.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_17.len() * std::mem::size_of::<f64>()))) }
);
    println!("{}", { let __sn_float_eq_right_18: &[f32] = &(vec![0.0]); let __sn_float_eq_left_18: &[f32] = &(positive); (__sn_float_eq_left_18.len() == __sn_float_eq_right_18.len() && __sn_float_eq_left_18.iter().flat_map(|value| value.to_ne_bytes()).eq(__sn_float_eq_right_18.iter().flat_map(|value| value.to_ne_bytes()).take(__sn_float_eq_left_18.len() * std::mem::size_of::<f32>()))) }
);
}
