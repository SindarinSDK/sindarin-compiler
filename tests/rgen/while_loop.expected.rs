#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut current: i64 = 0;
    let mut total: i64 = 0;
    while (current < 4) {
        { let __sn_place = &mut (current); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        (total = (total).checked_add(current).expect("checked arithmetic failed"));
    }
    println!("{}", total);
    let mut zero_iterations: i64 = 0;
    while (zero_iterations < 0) {
        (zero_iterations = 99);
    }
    println!("{}", zero_iterations);
}
