#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn increment(value: &mut i64) {
    (*(value) = (*(value)).checked_add(1).expect("checked arithmetic failed"));
}

fn twice(value: &mut i64) {
    increment(&mut *(value));
    { let __sn_previous = *(value); *(value) += 1; __sn_previous };
}

fn main() {
    let mut value: i64 = 1;
    twice(&mut (value));
    println!("{}", value);
}
