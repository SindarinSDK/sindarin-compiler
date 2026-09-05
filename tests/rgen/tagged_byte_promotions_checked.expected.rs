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
    println!("0x{:02X}", ({ let (__sn_byte_left, __sn_byte_right): (u8, u8) = (255, 1); __sn_byte_left.wrapping_add(__sn_byte_right) } as u32));
    println!("0x{:02X}", (-(1 as i32) as u32));
    println!("0x{:02X}", (!(1 as i32) as u32));
    println!("{}", ({ let (__sn_byte_left, __sn_byte_right): (u8, u8) = (255, 1); __sn_byte_left.wrapping_add(__sn_byte_right) } == 0));
    let mut stored: u8 = { let (__sn_byte_left, __sn_byte_right): (u8, u8) = (255, 1); __sn_byte_left.wrapping_add(__sn_byte_right) };
    println!("0x{:02X}", (stored as u32));
}
