#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

unsafe extern "C" {
    #[cfg(not(windows))]
    #[cfg_attr(target_vendor = "apple", link_name = "__stdoutp")]
    #[cfg_attr(not(target_vendor = "apple"), link_name = "stdout")]
    static mut __sn_c_stdout: *mut std::ffi::c_void;

    #[cfg(windows)]
    #[link_name = "__acrt_iob_func"]
    fn __sn_c_stdout(index: u32) -> *mut std::ffi::c_void;

    #[link_name = "fwrite"]
    fn __sn_c_fwrite(
        data: *const std::ffi::c_void,
        size: usize,
        count: usize,
        stream: *mut std::ffi::c_void,
    ) -> usize;
}

fn __sn_stdout_stream() -> *mut std::ffi::c_void {
    unsafe {
        #[cfg(windows)]
        { __sn_c_stdout(1) }
        #[cfg(not(windows))]
        { __sn_c_stdout }
    }
}

fn __sn_stdout_write(bytes: &[u8]) {
    if bytes.is_empty() { return; }
    let written = unsafe {
        __sn_c_fwrite(
            bytes.as_ptr().cast(), 1, bytes.len(),
            __sn_stdout_stream())
    };
    if written != bytes.len() { panic!("failed to write stdout"); }
}

macro_rules! print {
    ($($arg:tt)*) => {
        let rendered = format!($($arg)*);
        __sn_stdout_write(rendered.as_bytes());
    };
}

macro_rules! println {
    () => { __sn_stdout_write(b"\n") };
    ($($arg:tt)*) => {
        let mut rendered = format!($($arg)*);
        rendered.push('\n');
        __sn_stdout_write(rendered.as_bytes());
    };
}


fn __sn_runtime_error_0(message: &'static str) -> ! {
    eprintln!("{}", message);
    std::process::exit(1);
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

fn nextInt(calls: &mut i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return 5;
}

fn nextDouble(calls: &mut i64) -> f64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return 2.5;
}

fn markChar(trace: &mut i64) -> char {
    (*(trace) = __sn_checked_0((__sn_checked_0((*(trace)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(1), "Runtime error: integer overflow in addition"));
    return '\u{41}';
}

fn markDouble(trace: &mut i64) -> f64 {
    (*(trace) = __sn_checked_0((__sn_checked_0((*(trace)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(2), "Runtime error: integer overflow in addition"));
    return 65.5;
}

fn main() {
    let mut calls: i64 = 0;
    let mut sum: f64 = (((nextInt(&mut (calls))) as f64) + ((nextDouble(&mut (calls))) as f64));
    println!("{:.5}", sum);
    println!("{}", calls);
    let mut comparison: bool = (((nextInt(&mut (calls))) as i64) > ((nextDouble(&mut (calls))) as i64));
    println!("{}", comparison);
    println!("{}", calls);
    println!("{}", (((5) as f64) <= ((5.5) as f64)));
    println!("{}", (((5) as f64) >= ((5.5) as f64)));
    println!("{}", (((5) as f64) != ((5.5) as f64)));
    println!("{:.5}", (((5) as f64) % ((2.5) as f64)));
    let mut widened: f64 = ((nextInt(&mut (calls))) as f64);
    println!("{:.5}", widened);
    println!("{}", calls);
    (calls = 0);
    println!("{}", ((((markChar(&mut (calls))) as u32) as f64) == ((markDouble(&mut (calls))) as f64)));
    println!("{}", calls);
    (calls = 0);
    println!("{:.5}", ((((markChar(&mut (calls))) as u32) as f64) + ((markDouble(&mut (calls))) as f64)));
    println!("{}", calls);
}
