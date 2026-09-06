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

fn markByte(trace: &mut i64, marker: i64, value: u8) -> u8 {
    (*(trace) = __sn_checked_0((__sn_checked_0((*(trace)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    return value;
}

fn markInt(trace: &mut i64, marker: i64, value: i64) -> i64 {
    (*(trace) = __sn_checked_0((__sn_checked_0((*(trace)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    return value;
}

fn main() {
    let mut trace: i64 = 0;
    let mut mixed: i64 = { let (__sn_left, __sn_right): (i64, i64) = ((markByte(&mut (trace), 1, 255)) as i64, (markInt(&mut (trace), 2, 1000)) as i64); __sn_checked_0(__sn_left.checked_add(__sn_right), "Runtime error: integer overflow in addition") };
    println!("{}", mixed);
    println!("{}", trace);
    (trace = 0);
    let mut stored: i64 = (({ let (__sn_byte_left, __sn_byte_right): (u8, u8) = (markByte(&mut (trace), 1, 200), markByte(&mut (trace), 2, 200)); __sn_byte_left.wrapping_mul(__sn_byte_right) }) as i64);
    println!("{}", stored);
    println!("{}", trace);
    let mut total: i64 = 0;
    let mut value: u8 = 255;
    (total = { let (__sn_left, __sn_right): (i64, i64) = ((total) as i64, (value) as i64); __sn_checked_0(__sn_left.checked_add(__sn_right), "Runtime error: integer overflow in addition") });
    println!("{}", total);
}
