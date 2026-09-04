#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn observeSubject(calls: &mut i64, order: &mut i64, value: bool) -> bool {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = ((*(order)).checked_mul(10).expect("checked arithmetic failed")).checked_add(1).expect("checked arithmetic failed"));
    return value;
}

fn main() {
    let mut subject_calls: i64 = 0;
    let mut order: i64 = 0;
    let mut first: i64 = 0;
    match (observeSubject(&mut (subject_calls), &mut (order), true)) {
        true => {
            (order = ((order).checked_mul(10).expect("checked arithmetic failed")).checked_add(2).expect("checked arithmetic failed"));
            (first = 10);
        },
        true => {
            (order = ((order).checked_mul(10).expect("checked arithmetic failed")).checked_add(3).expect("checked arithmetic failed"));
            (first = 20);
        },
        _ => {
            (first = 30);
        },
    };
    let mut true_hit: bool = false;
    match (true) {
        true => {
            (true_hit = true);
        },
        false => {
            (true_hit = false);
        },
        _ => {},
    };
    let mut false_hit: bool = false;
    match (false) {
        true => {
            (false_hit = false);
        },
        false => {
            (false_hit = true);
        },
        _ => {},
    };
    let mut fallback: i64 = 0;
    match (false) {
        true => {
            (fallback = 1);
        },
        _ => {
            (fallback = 7);
        },
    };
    let mut unchanged: i64 = 11;
    match (false) {
        true => {
            (unchanged = 99);
        },
        _ => {},
    };
    let mut alternatives: i64 = 0;
    match (false) {
        true | false => {
            (alternatives = 1);
        },
        _ => {
            (alternatives = 2);
        },
    };
    let mut nested: i64 = 0;
    match (true) {
        true => {
            match (false) {
        false => {
            (nested = 5);
        },
        _ => {},
    };
        },
        _ => {},
    };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", subject_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", first)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", true_hit)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", false_hit)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", fallback)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", unchanged)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", alternatives)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", nested)); __sn_interpolated });
}
