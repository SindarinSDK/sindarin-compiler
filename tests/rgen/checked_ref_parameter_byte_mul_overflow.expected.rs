#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn fail(value: &mut u8) {
    let mut two: u8 = 2;
    { let __sn_rhs = two; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_mul(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
}

fn main() {
    let mut value: u8 = 128;
    fail(&mut (value));
}
