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
    let mut byte_value: u8 = 5;
    { let (__sn_rhs, __sn_place) = (2, &mut (byte_value)); let (__sn_left, __sn_right): (i64, i64) = (*__sn_place as i64, __sn_rhs as i64); let __sn_promoted = __sn_checked_0(__sn_left.checked_add(__sn_right), "Runtime error: integer overflow in addition"); let __sn_next = __sn_promoted as u8; *__sn_place = __sn_next; __sn_next };
    println!("0x{:02X}", (byte_value as u32));
    let mut int_value: i64 = 5;
    { let (__sn_rhs, __sn_place) = (2, &mut (int_value)); let (__sn_left, __sn_right): (i64, i64) = (*__sn_place as i64, __sn_rhs as i64); let __sn_promoted = __sn_checked_0(__sn_left.checked_add(__sn_right), "Runtime error: integer overflow in addition"); let __sn_next = __sn_promoted as i64; *__sn_place = __sn_next; __sn_next };
    println!("{}", int_value);
    let mut int32_value: i32 = 6;
    { let (__sn_rhs, __sn_place) = (8, &mut (int32_value)); let (__sn_left, __sn_right): (i64, i64) = (*__sn_place as i64, __sn_rhs as i64); let __sn_promoted = __sn_checked_0(__sn_left.checked_mul(__sn_right), "Runtime error: integer overflow in multiplication"); let __sn_next = __sn_promoted as i32; *__sn_place = __sn_next; __sn_next };
    println!("{}", int32_value);
}
