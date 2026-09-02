#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn fail(value: &mut u64) {
    let mut one: u64 = 1;
    { let __sn_rhs = one; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
}

fn main() {
    let mut half: u64 = 9223372036854775807;
    let mut max_minus_one: u64 = (half).checked_mul(2).expect("checked arithmetic failed");
    let mut max: u64 = (max_minus_one).checked_add(1).expect("checked arithmetic failed");
    fail(&mut (max));
}
