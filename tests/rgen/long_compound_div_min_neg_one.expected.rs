#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut base: i64 = (-9223372036854775807);
    let mut one: i64 = 1;
    let mut minimum: i64 = (base).checked_sub(one).expect("checked arithmetic failed");
    let mut negative_one: i64 = (-1);
    { let __sn_rhs = negative_one; let __sn_place = &mut (minimum); let __sn_next = (*__sn_place).checked_div(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
}
