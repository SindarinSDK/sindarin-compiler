#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn increment(value: &mut i64) -> i64 {
    return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
}

fn decrement(value: &mut i64) -> i64 {
    return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_sub(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
}

fn main() {
    let mut high: i64 = 9;
    let mut low: i64 = 5;
    println!("{}", increment(&mut (high)));
    println!("{}", decrement(&mut (low)));
    println!("{}", high);
    println!("{}", low);
}
