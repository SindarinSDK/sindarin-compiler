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
    let mut max_minus_one: u64 = __sn_checked_0((half).checked_mul(two), "Runtime error: integer overflow in multiplication");
    let mut max: u64 = __sn_checked_0((max_minus_one).checked_add(one), "Runtime error: integer overflow in addition");
    let mut sum: u64 = __sn_checked_0((max_minus_one).checked_add(one), "Runtime error: integer overflow in addition");
    let mut difference: u64 = __sn_checked_0((one).checked_sub(one), "Runtime error: integer overflow in subtraction");
    let mut product: u64 = __sn_checked_0((half).checked_mul(two), "Runtime error: integer overflow in multiplication");
    let mut quotient: u64 = { let __sn_left = max; let __sn_right = one; __sn_checked_div_0(__sn_left.checked_div(__sn_right), __sn_right == 0) };
    let mut remainder: u64 = { let __sn_left = max; let __sn_right = two; __sn_checked_mod_0(__sn_left.checked_rem(__sn_right), __sn_right == 0) };
    if (((((sum == max) && (difference == 0)) && (product == max_minus_one)) && (quotient == max)) && (remainder == one)) {
        println!("{}", "ok".to_string());
    } else {
        println!("{}", "wrong".to_string());
    }
}
