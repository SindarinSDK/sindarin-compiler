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

fn fail(value: &mut u64) {
    { let __sn_byte_place = &mut (*(value)); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous };
}

fn main() {
    let mut half: u64 = 9223372036854775807;
    let mut max_minus_one: u64 = { let (__sn_byte_left, __sn_byte_right): (u64, u64) = (half, 2); __sn_byte_left.wrapping_mul(__sn_byte_right) };
    let mut max: u64 = { let (__sn_byte_left, __sn_byte_right): (u64, u64) = (max_minus_one, 1); __sn_byte_left.wrapping_add(__sn_byte_right) };
    fail(&mut (max));
}
