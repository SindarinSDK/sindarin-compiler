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

fn main() {
    let mut min_base: i32 = (-2147483647);
    let mut one: i32 = 1;
    let mut minimum: i32 = __sn_checked_0((min_base).checked_sub(one), "Runtime error: integer overflow in subtraction");
    let mut negative_one: i32 = (-1);
    let mut quotient: i32 = { let __sn_left = minimum; let __sn_right = negative_one; __sn_checked_div_0(__sn_left.checked_div(__sn_right), __sn_right == 0) };
}
