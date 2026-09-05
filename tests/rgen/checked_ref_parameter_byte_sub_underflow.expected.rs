#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn fail(value: &mut u8) {
    let mut one: u8 = 1;
    { let __sn_rhs = one; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
}

fn main() {
    let mut value: u8 = 0;
    fail(&mut (value));
}
