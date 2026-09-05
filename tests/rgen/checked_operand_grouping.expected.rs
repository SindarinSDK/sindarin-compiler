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
    let mut a: i64 = 2;
    let mut b: i64 = 3;
    let mut negative_literal: i64 = (-2);
    let mut negative_sum: i64 = __sn_checked_0((negative_literal).checked_add(3), "Runtime error: integer overflow in addition");
    let mut negated: i64 = __sn_checked_0(((-a)).checked_add(b), "Runtime error: integer overflow in addition");
    let mut nested: i64 = __sn_checked_0((__sn_checked_0((a).checked_add(b), "Runtime error: integer overflow in addition")).checked_mul(2), "Runtime error: integer overflow in multiplication");
    print!("{}", negative_sum);
    print!("{}", ",".to_string());
    print!("{}", negated);
    print!("{}", ",".to_string());
    print!("{}", nested);
}
