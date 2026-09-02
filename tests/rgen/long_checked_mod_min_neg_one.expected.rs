#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut base: i64 = (-9223372036854775807);
    let mut one: i64 = 1;
    let mut minimum: i64 = (base).checked_sub(one).expect("checked arithmetic failed");
    let mut negative_one: i64 = (-1);
    let mut overflow: i64 = (minimum).checked_rem(negative_one).expect("checked arithmetic failed");
}
