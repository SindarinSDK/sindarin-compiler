#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut half: u64 = 9223372036854775807;
    let mut two: u64 = 2;
    let mut one: u64 = 1;
    let mut max_minus_one: u64 = (half).checked_mul(two).expect("checked arithmetic failed");
    let mut max: u64 = (max_minus_one).checked_add(one).expect("checked arithmetic failed");
    let mut sum: u64 = (max_minus_one).checked_add(one).expect("checked arithmetic failed");
    let mut difference: u64 = (one).checked_sub(one).expect("checked arithmetic failed");
    let mut product: u64 = (half).checked_mul(two).expect("checked arithmetic failed");
    let mut quotient: u64 = (max).checked_div(one).expect("checked arithmetic failed");
    let mut remainder: u64 = (max).checked_rem(two).expect("checked arithmetic failed");
    if (((((sum == max) && (difference == 0)) && (product == max_minus_one)) && (quotient == max)) && (remainder == one)) {
        println!("{}", "ok".to_string());
    } else {
        println!("{}", "wrong".to_string());
    }
}
