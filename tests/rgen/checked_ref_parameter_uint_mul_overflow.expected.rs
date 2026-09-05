#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn fail(value: &mut u64) {
    let mut two: u64 = 2;
    { let __sn_rhs = two; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_mul(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
}

fn main() {
    let mut half: u64 = 9223372036854775807;
    let mut high: u64 = (half).checked_add(1).expect("checked arithmetic failed");
    fail(&mut (high));
}
