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

fn bumpAndReturn(counter: &mut i64) -> u64 {
    { let __sn_rhs = 1; let __sn_place = &mut (*(counter)); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    return 42;
}

fn main() {
    let mut zero: u64 = 0;
    let mut one: u64 = 1;
    let mut representative: u64 = 42;
    let mut exact: u64 = 9007199254740991;
    let mut half: u64 = 9223372036854775807;
    let mut max_minus_one: u64 = { let (__sn_byte_left, __sn_byte_right): (u64, u64) = (half, 2); __sn_byte_left.wrapping_mul(__sn_byte_right) };
    let mut max: u64 = { let (__sn_byte_left, __sn_byte_right): (u64, u64) = (max_minus_one, 1); __sn_byte_left.wrapping_add(__sn_byte_right) };
    let mut counter: i64 = 0;
    let mut called: f64 = (bumpAndReturn(&mut (counter)) as f64);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((((((zero as f64) == 0.0) && ((one as f64) == 1.0)) && ((representative as f64) == 42.0)) && ((exact as f64) == 9007199254740991.0)) && ((max as f64) == ((half as f64) * 2.0))))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", ((counter == 1) && (called == 42.0)))); __sn_interpolated });
}
