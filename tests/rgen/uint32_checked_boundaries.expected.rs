#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut max: u32 = 4294967295;
    let mut one: u32 = 1;
    let mut add_base: u32 = 4294967294;
    let mut sum: u32 = (add_base).checked_add(one).expect("checked arithmetic failed");
    let mut difference: u32 = (one).checked_sub(one).expect("checked arithmetic failed");
    let mut mul_left: u32 = 65535;
    let mut mul_right: u32 = 65537;
    let mut product: u32 = (mul_left).checked_mul(mul_right).expect("checked arithmetic failed");
    let mut quotient: u32 = (max).checked_div(one).expect("checked arithmetic failed");
    let mut remainder: u32 = (max).checked_rem(2).expect("checked arithmetic failed");
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", sum)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", difference)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", product)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", quotient)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", remainder)); __sn_interpolated });
}
