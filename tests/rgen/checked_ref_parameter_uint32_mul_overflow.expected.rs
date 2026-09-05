#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn fail(value: &mut u32) {
    let mut two: u32 = 2;
    { let __sn_rhs = two; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_mul(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
}

fn main() {
    let mut value: u32 = 2147483648;
    fail(&mut (value));
}
