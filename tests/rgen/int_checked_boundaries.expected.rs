#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut add_left: i64 = 9223372036854775806;
    let mut add_right: i64 = 1;
    let mut sum: i64 = (add_left).checked_add(add_right).expect("checked arithmetic failed");
    let mut min_base: i64 = (-9223372036854775807);
    let mut min_step: i64 = 1;
    let mut minimum: i64 = (min_base).checked_sub(min_step).expect("checked arithmetic failed");
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", sum)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", minimum)); __sn_interpolated });
}
