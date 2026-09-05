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
    let mut max: u32 = 4294967295;
    let mut one: u32 = 1;
    let mut add_base: u32 = 4294967294;
    let mut sum: u32 = __sn_checked_0((add_base).checked_add(one), "Runtime error: integer overflow in addition");
    let mut difference: u32 = __sn_checked_0((one).checked_sub(one), "Runtime error: integer overflow in subtraction");
    let mut mul_left: u32 = 65535;
    let mut mul_right: u32 = 65537;
    let mut product: u32 = __sn_checked_0((mul_left).checked_mul(mul_right), "Runtime error: integer overflow in multiplication");
    let mut quotient: u32 = { let __sn_left = max; let __sn_right = one; __sn_checked_div_0(__sn_left.checked_div(__sn_right), __sn_right == 0) };
    let mut remainder: u32 = { let __sn_left = max; let __sn_right = 2; __sn_checked_mod_0(__sn_left.checked_rem(__sn_right), __sn_right == 0) };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", sum)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", difference)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", product)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", quotient)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", remainder)); __sn_interpolated });
}
