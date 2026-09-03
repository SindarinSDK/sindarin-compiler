#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Pair {
    left: i64,
    right: i64,
}

fn sum(a: i64, b: i64) -> i64 {
    return (a).checked_add(b).expect("checked arithmetic failed");
}

fn main() {
    let mut pair: Pair = Pair { left: 19, right: 23 };
    println!("{}", sum((pair).left, (pair).right));
}
