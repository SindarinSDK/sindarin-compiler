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

fn main() {
    let mut add_base: i32 = 2147483646;
    let mut one: i32 = 1;
    let mut sum: i32 = __sn_checked((add_base).checked_add(one), "Runtime error: integer overflow in addition")
;
    let mut min_base: i32 = (-2147483647);
    let mut minimum: i32 = __sn_checked((min_base).checked_sub(one), "Runtime error: integer overflow in subtraction")
;
    let mut mul_base: i32 = (-1073741824);
    let mut two: i32 = 2;
    let mut product: i32 = __sn_checked((mul_base).checked_mul(two), "Runtime error: integer overflow in multiplication")
;
    let mut quotient: i32 = { let __sn_left = minimum; let __sn_right = one; __sn_checked_div(__sn_left.checked_div(__sn_right), __sn_right == 0) }
;
    let mut remainder: i32 = { let __sn_left = minimum; let __sn_right = one; __sn_checked_mod(__sn_left.checked_rem(__sn_right), __sn_right == 0) }
;
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", sum)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", minimum)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", product)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", quotient)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", remainder)); __sn_interpolated });
}

