#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn fail(value: &mut i64) {
    { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = (*__sn_place).checked_mul(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
}

fn main() {
    let mut value: i64 = 4611686018427387904;
    fail(&mut (value));
}
