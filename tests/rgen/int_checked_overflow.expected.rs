#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut max: i64 = 9223372036854775807;
    let mut one: i64 = 1;
    let mut overflow: i64 = (max).checked_add(one).expect("checked arithmetic failed");
}
