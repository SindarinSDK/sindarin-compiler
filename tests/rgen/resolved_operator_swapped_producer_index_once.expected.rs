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

#[derive(Clone, Copy, Debug, PartialEq)]
struct Item {
    value: i64,
}

impl Item {
    fn op_lt(&self, other: &mut Item) -> bool {
        ((other).value = __sn_checked_0(((other).value).checked_add(1), "Runtime error: integer overflow in addition"));
        return ((self).value < (other).value);
    }
}
#[derive(Clone, Debug, PartialEq)]
struct Bucket {
    items: Vec<Item>,
}

fn makeReceiver(item: Item, trace: &mut i64, digit: i64) -> Item {
    (*(trace) = __sn_checked_0((__sn_checked_0((*(trace)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(digit), "Runtime error: integer overflow in addition"));
    return Item { value: __sn_checked_0(((item).value).checked_add(1), "Runtime error: integer overflow in addition") };
}

fn makeItems(calls: &mut i64, trace: &mut i64, digit: i64) -> Vec<Item> {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(trace) = __sn_checked_0((__sn_checked_0((*(trace)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(digit), "Runtime error: integer overflow in addition"));
    return vec![Item { value: 1 }];
}

fn makeBucket(calls: &mut i64, trace: &mut i64, digit: i64) -> Bucket {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(trace) = __sn_checked_0((__sn_checked_0((*(trace)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(digit), "Runtime error: integer overflow in addition"));
    return Bucket { items: vec![Item { value: 1 }] };
}

fn makeBuckets(calls: &mut i64, trace: &mut i64, digit: i64) -> Vec<Bucket> {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(trace) = __sn_checked_0((__sn_checked_0((*(trace)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(digit), "Runtime error: integer overflow in addition"));
    return vec![Bucket { items: vec![Item { value: 1 }] }];
}

fn index(calls: &mut i64, trace: &mut i64, digit: i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(trace) = __sn_checked_0((__sn_checked_0((*(trace)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(digit), "Runtime error: integer overflow in addition"));
    return 0;
}

fn main() {
    let mut seed: Item = Item { value: 1 };
    let mut producerCalls: i64 = 0;
    let mut indexCalls: i64 = 0;
    let mut trace: i64 = 0;
    println!("{}", { let __sn_resolved_receiver_3 = & (makeReceiver(seed, &mut (trace), 1)); let __sn_resolved_array_0 = &mut (makeItems(&mut (producerCalls), &mut (trace), 2)); let __sn_resolved_index_1 = __sn_index((__sn_resolved_array_0).len(), index(&mut (indexCalls), &mut (trace), 3)); let __sn_resolved_arg_2 = &mut (__sn_resolved_array_0)[__sn_resolved_index_1];(__sn_resolved_receiver_3).op_lt(__sn_resolved_arg_2) });
    println!("{}", producerCalls);
    println!("{}", indexCalls);
    println!("{}", trace);
    (producerCalls = 0);
    (indexCalls = 0);
    (trace = 0);
    println!("{}", { let __sn_resolved_receiver_7 = & (makeReceiver(seed, &mut (trace), 1)); let __sn_resolved_array_4 = &mut ((makeBucket(&mut (producerCalls), &mut (trace), 2)).items); let __sn_resolved_index_5 = __sn_index((__sn_resolved_array_4).len(), index(&mut (indexCalls), &mut (trace), 3)); let __sn_resolved_arg_6 = &mut (__sn_resolved_array_4)[__sn_resolved_index_5];(__sn_resolved_receiver_7).op_lt(__sn_resolved_arg_6) });
    println!("{}", producerCalls);
    println!("{}", indexCalls);
    println!("{}", trace);
    (producerCalls = 0);
    (indexCalls = 0);
    (trace = 0);
    println!("{}", { let __sn_resolved_receiver_13 = & (makeReceiver(seed, &mut (trace), 1)); let mut __sn_resolved_owner_8: Vec<Bucket> = makeBuckets(&mut (producerCalls), &mut (trace), 2);let __sn_resolved_place_index_9 = __sn_index((__sn_resolved_owner_8).len(), index(&mut (indexCalls), &mut (trace), 3));let __sn_resolved_array_10 = &mut (((__sn_resolved_owner_8)[__sn_resolved_place_index_9]).items); let __sn_resolved_index_11 = __sn_index((__sn_resolved_array_10).len(), index(&mut (indexCalls), &mut (trace), 4)); let __sn_resolved_arg_12 = &mut (__sn_resolved_array_10)[__sn_resolved_index_11];(__sn_resolved_receiver_13).op_lt(__sn_resolved_arg_12) });
    println!("{}", producerCalls);
    println!("{}", indexCalls);
    println!("{}", trace);
}
