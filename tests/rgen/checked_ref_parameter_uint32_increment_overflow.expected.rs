#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn fail(value: &mut u32) {
    { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
}

fn main() {
    let mut value: u32 = 4294967295;
    fail(&mut (value));
}
