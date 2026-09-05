#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn operand(trace: &mut i64, value: u8) -> u8 {
    println!("{}", *(trace));
    { let __sn_rhs = 1; let __sn_place = &mut (*(trace)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    return value;
}

fn main() {
    let mut trace: i64 = 1;
    let mut result: u8 = { let (__sn_byte_left, __sn_byte_right): (u8, u8) = (operand(&mut (trace), 255), operand(&mut (trace), 1)); __sn_byte_left.wrapping_add(__sn_byte_right) };
    println!("{}", trace);
    println!("{}", (result == 0));
}
