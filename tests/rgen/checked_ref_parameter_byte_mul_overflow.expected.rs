#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn __sn_runtime_error(message: &'static str) -> ! {
    eprintln!("{}", message);
    std::process::exit(1);
}

fn __sn_checked<T>(value: Option<T>, message: &'static str) -> T {
    match value {
        Some(value) => value,
        None => __sn_runtime_error(message),
    }
}

fn __sn_checked_div<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked(value, if divisor_is_zero {
        "panic: Division by zero"
    } else {
        "Runtime error: integer overflow in division"
    })
}

fn __sn_checked_mod<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked(value, if divisor_is_zero {
        "panic: Modulo by zero"
    } else {
        "Runtime error: integer overflow in modulo"
    })
}

fn fail(value: &mut u8) {
    let mut two: u8 = 2;
    { let __sn_rhs = two; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
}

fn main() {
    let mut value: u8 = 128;
    fail(&mut (value));
}

