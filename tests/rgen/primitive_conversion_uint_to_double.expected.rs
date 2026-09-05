#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn bumpAndReturn(counter: &mut i64) -> u64 {
    { let __sn_rhs = 1; let __sn_place = &mut (*(counter)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    return 42;
}

fn main() {
    let mut zero: u64 = 0;
    let mut one: u64 = 1;
    let mut representative: u64 = 42;
    let mut exact: u64 = 9007199254740991;
    let mut half: u64 = 9223372036854775807;
    let mut max_minus_one: u64 = (half).checked_mul(2).expect("checked arithmetic failed");
    let mut max: u64 = (max_minus_one).checked_add(1).expect("checked arithmetic failed");
    let mut counter: i64 = 0;
    let mut called: f64 = (bumpAndReturn(&mut (counter)) as f64);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((((((zero as f64) == 0.0) && ((one as f64) == 1.0)) && ((representative as f64) == 42.0)) && ((exact as f64) == 9007199254740991.0)) && ((max as f64) == ((half as f64) * 2.0))))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", ((counter == 1) && (called == 42.0)))); __sn_interpolated });
}
