#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Box {
    value: u8,
}

fn main() {
    let mut r#box: Box = Box { value: 255 };
    { let __sn_place = &mut ((r#box).value); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
}
