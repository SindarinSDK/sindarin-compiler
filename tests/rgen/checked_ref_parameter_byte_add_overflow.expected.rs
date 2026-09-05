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

fn fail(value: &mut u8) {
    let mut one: u8 = 1;
    { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (one, &mut (*(value))); let __sn_byte_next = (*__sn_byte_place).wrapping_add(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };
}

fn main() {
    let mut value: u8 = 255;
    fail(&mut (value));
}
