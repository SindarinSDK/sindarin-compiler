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
    let mut add_base: i32 = 2147483646;
    let mut one: i32 = 1;
    let mut sum: i32 = { let (__sn_byte_left, __sn_byte_right): (i32, i32) = (add_base, one); __sn_byte_left.wrapping_add(__sn_byte_right) };
    let mut min_base: i32 = (-2147483647);
    let mut minimum: i32 = { let (__sn_byte_left, __sn_byte_right): (i32, i32) = (min_base, one); __sn_byte_left.wrapping_sub(__sn_byte_right) };
    let mut mul_base: i32 = (-1073741824);
    let mut two: i32 = 2;
    let mut product: i32 = { let (__sn_byte_left, __sn_byte_right): (i32, i32) = (mul_base, two); __sn_byte_left.wrapping_mul(__sn_byte_right) };
    let mut quotient: i32 = { let __sn_left = minimum; let __sn_right = one; __sn_checked_div_0(__sn_left.checked_div(__sn_right), __sn_right == 0) };
    let mut remainder: i32 = { let __sn_left = minimum; let __sn_right = one; __sn_checked_mod_0(__sn_left.checked_rem(__sn_right), __sn_right == 0) };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", sum)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", minimum)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", product)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", quotient)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", remainder)); __sn_interpolated });
}
