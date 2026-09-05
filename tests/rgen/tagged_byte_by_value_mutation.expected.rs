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

fn mutate(mut value: u8) -> u8 {
    { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (2, &mut (value)); let __sn_byte_next = (*__sn_byte_place).wrapping_add(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };
    { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (2, &mut (value)); let __sn_byte_next = (*__sn_byte_place).wrapping_mul(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };
    let mut before: u8 = { let __sn_byte_place = &mut (value); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous };
    { let __sn_byte_place = &mut (value); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_sub(1); __sn_byte_previous };
    return { let (__sn_byte_left, __sn_byte_right): (u8, u8) = (before, value); __sn_byte_left.wrapping_add(__sn_byte_right) };
}

fn main() {
    let mut original: u8 = 255;
    println!("{}", (mutate(original) == 4));
    println!("{}", (original == 255));
}
