#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut max: i64 = 9223372036854775807;
    let mut one: i64 = 1;
    { let __sn_rhs = one; let __sn_place = &mut (max); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
}
