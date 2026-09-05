#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn fail(value: &mut u64) {
    { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
}

fn main() {
    let mut half: u64 = 9223372036854775807;
    let mut max_minus_one: u64 = (half).checked_mul(2).expect("checked arithmetic failed");
    let mut max: u64 = (max_minus_one).checked_add(1).expect("checked arithmetic failed");
    fail(&mut (max));
}
