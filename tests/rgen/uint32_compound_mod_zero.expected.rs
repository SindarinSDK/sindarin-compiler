#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut value: u32 = 1;
    let mut zero: u32 = 0;
    { let __sn_rhs = zero; let __sn_place = &mut (value); let __sn_next = (*__sn_place).checked_rem(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
}
