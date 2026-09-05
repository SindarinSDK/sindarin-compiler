#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn add(a: i64, b: i64) -> i64 {
    return (a).checked_add(b).expect("checked arithmetic failed");
}

fn main() {
    let mut result: i64 = add(20, 22);
    println!("{}", result);
}
