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

fn operand(trace: &mut i64, value: u8) -> u8 {
    println!("{}", *(trace));
    { let __sn_rhs = 1; let __sn_place = &mut (*(trace)); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    return value;
}

fn main() {
    let mut trace: i64 = 1;
    let mut result: u8 = { let (__sn_byte_left, __sn_byte_right): (u8, u8) = (operand(&mut (trace), 255), operand(&mut (trace), 1)); __sn_byte_left.wrapping_add(__sn_byte_right) };
    println!("{}", trace);
    println!("{}", (result == 0));
}
