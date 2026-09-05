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
    let mut half: u64 = 9223372036854775807;
    let mut two: u64 = 2;
    let mut one: u64 = 1;
    let mut max_minus_one: u64 = { let (__sn_byte_left, __sn_byte_right): (u64, u64) = (half, two); __sn_byte_left.wrapping_mul(__sn_byte_right) };
    let mut max: u64 = { let (__sn_byte_left, __sn_byte_right): (u64, u64) = (max_minus_one, one); __sn_byte_left.wrapping_add(__sn_byte_right) };
    let mut zero: u64 = 0;
    let mut quotient: u64 = { let __sn_left = max; let __sn_right = zero; __sn_checked_div_0(__sn_left.checked_div(__sn_right), __sn_right == 0) };
}
