#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut max: u8 = 255;
    let mut one: u8 = 1;
    let mut overflow: u8 = (max).checked_add(one).expect("checked arithmetic failed");
}
