#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut zero: u64 = 0;
    let mut one: u64 = 1;
    { let __sn_rhs = one; let __sn_place = &mut (zero); let __sn_next = (*__sn_place).checked_sub(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
}
