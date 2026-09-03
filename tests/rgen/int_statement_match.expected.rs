#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn nextSubject(calls: &mut i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    return 2;
}

fn main() {
    let mut __sn_match_subject: i64 = 2;
    let mut subject_calls: i64 = 0;
    let mut selected: i64 = 0;
    let mut effects: i64 = 0;
    match (nextSubject(&mut (subject_calls)) as i64) {
        1 | 2 => {
            (selected = 10);
            { let __sn_place = &mut (effects); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
        },
        2 | 3 => {
            (selected = 20);
            { let __sn_rhs = 100; let __sn_place = &mut (effects); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        },
        _ => {
            (selected = 30);
            { let __sn_rhs = 1000; let __sn_place = &mut (effects); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        },
    };
    let mut fallback: i64 = 0;
    match (99 as i64) {
        1 | 2 => {
            (fallback = 1);
        },
        _ => {
            (fallback = 7);
        },
    };
    let mut unchanged: i64 = 11;
    match (42 as i64) {
        1 | 2 => {
            (unchanged = 99);
        },
        _ => {},
    };
    let mut negative: i64 = 0;
    match ((-7) as i64) {
        (-8) | (-7) => {
            (negative = 1);
        },
        _ => {
            (negative = 2);
        },
    };
    let mut upper_bound: i64 = 0;
    match (9223372036854775807 as i64) {
        (-9223372036854775807) | 9223372036854775807 => {
            (upper_bound = 1);
        },
        _ => {
            (upper_bound = 2);
        },
    };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", __sn_match_subject)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", subject_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", selected)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", effects)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", fallback)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", unchanged)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", negative)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", upper_bound)); __sn_interpolated });
}
