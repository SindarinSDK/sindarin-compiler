#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut left: i64 = 3037000500;
    let mut right: i64 = 3037000500;
    let mut overflow: i64 = (left).checked_mul(right).expect("checked arithmetic failed");
}
