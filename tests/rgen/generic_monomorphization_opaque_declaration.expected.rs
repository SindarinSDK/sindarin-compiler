#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    println!("{}", identity_int(42));
}

fn identity_int(value: i64) -> i64 {
    return value;
}
