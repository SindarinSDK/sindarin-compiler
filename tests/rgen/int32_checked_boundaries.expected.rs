#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut add_base: i32 = 2147483646;
    let mut one: i32 = 1;
    let mut sum: i32 = (add_base).checked_add(one).expect("checked arithmetic failed");
    let mut min_base: i32 = (-2147483647);
    let mut minimum: i32 = (min_base).checked_sub(one).expect("checked arithmetic failed");
    let mut mul_base: i32 = (-1073741824);
    let mut two: i32 = 2;
    let mut product: i32 = (mul_base).checked_mul(two).expect("checked arithmetic failed");
    let mut quotient: i32 = (minimum).checked_div(one).expect("checked arithmetic failed");
    let mut remainder: i32 = (minimum).checked_rem(one).expect("checked arithmetic failed");
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", sum)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", minimum)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", product)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", quotient)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", remainder)); __sn_interpolated });
}
