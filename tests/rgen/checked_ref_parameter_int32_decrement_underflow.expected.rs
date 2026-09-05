#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn fail(value: &mut i32) {
    { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
}

fn main() {
    let mut min_base: i32 = (-2147483647);
    let mut one: i32 = 1;
    let mut value: i32 = (min_base).checked_sub(one).expect("checked arithmetic failed");
    fail(&mut (value));
}
