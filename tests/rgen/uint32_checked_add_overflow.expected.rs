#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut max: u32 = 4294967295;
    let mut one: u32 = 1;
    let mut overflow: u32 = (max).checked_add(one).expect("checked arithmetic failed");
}
