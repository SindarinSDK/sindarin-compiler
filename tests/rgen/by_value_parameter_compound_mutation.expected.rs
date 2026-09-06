#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn increment(mut value: i64) {
    { let (__sn_rhs, __sn_place): (i64, &mut i64) = (1, &mut (value)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
}

fn main() {
    increment(1);
}
