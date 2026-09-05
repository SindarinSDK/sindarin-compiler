#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn left__score() -> i64 {
    return 5;
}

fn right__score() -> i64 {
    return 7;
}

fn main() {
    println!("{}", (left__score()).checked_add(right__score()).expect("checked arithmetic failed"));
}
