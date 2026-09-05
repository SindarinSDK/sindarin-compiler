#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

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
struct ReceiverInner {
    value: i64,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct ReceiverOuter {
    inner: ReceiverInner,
    count: i64,
}

impl ReceiverOuter {
    unsafe fn touch(__sn_raw_self_1: *mut Self, value: *mut i64) -> i64 {
        ((*(__sn_raw_self_1)).count = __sn_checked_0(((*(__sn_raw_self_1)).count).checked_add(1), "Runtime error: integer overflow in addition"));
        (*(value) = __sn_checked_0((*(value)).checked_add(10), "Runtime error: integer overflow in addition"));
        return __sn_checked_0(((*(__sn_raw_self_1)).count).checked_add(*(value)), "Runtime error: integer overflow in addition");
    }
}

fn main() {
    let mut __sn_raw_self_0: i64 = 5;
    println!("{}", __sn_raw_self_0);
    let mut outer: ReceiverOuter = ReceiverOuter { inner: ReceiverInner { value: 1 }, count: 0 };
    println!("{}", unsafe { ReceiverOuter::touch(std::ptr::addr_of_mut!(outer), std::ptr::addr_of_mut!(((outer).inner).value)) });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (outer).count)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((outer).inner).value)); __sn_interpolated });
}
