#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut max: u8 = 255;
    let mut one: u8 = 1;
    let mut add_base: u8 = 254;
    let mut sum: u8 = (add_base).checked_add(one).expect("checked arithmetic failed");
    let mut difference: u8 = (one).checked_sub(one).expect("checked arithmetic failed");
    let mut product: u8 = (max).checked_mul(one).expect("checked arithmetic failed");
    let mut quotient: u8 = (max).checked_div(one).expect("checked arithmetic failed");
    let mut remainder: u8 = (max).checked_rem(one).expect("checked arithmetic failed");
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", sum)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", difference)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", product)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", quotient)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", remainder)); __sn_interpolated });
}
