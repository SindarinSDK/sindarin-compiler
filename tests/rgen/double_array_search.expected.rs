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
    let mut zero: f64 = 0.0;
    let mut negative_zero: f64 = (-0.0);
    let mut nan: f64 = (zero / zero);
    let mut values: Vec<f64> = vec![zero, 1.5, nan, 1.5];
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (1.5 as f64).to_bits(); __sn_array.iter().any(|__sn_item| __sn_item.to_bits() == __sn_array_search) });
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (1.5 as f64).to_bits(); __sn_array.iter().position(|__sn_item| __sn_item.to_bits() == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (negative_zero as f64).to_bits(); __sn_array.iter().any(|__sn_item| __sn_item.to_bits() == __sn_array_search) });
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (negative_zero as f64).to_bits(); __sn_array.iter().position(|__sn_item| __sn_item.to_bits() == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (nan as f64).to_bits(); __sn_array.iter().any(|__sn_item| __sn_item.to_bits() == __sn_array_search) });
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (nan as f64).to_bits(); __sn_array.iter().position(|__sn_item| __sn_item.to_bits() == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
}
