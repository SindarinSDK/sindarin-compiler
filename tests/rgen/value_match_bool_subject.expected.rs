#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn observeSubject(calls: &mut i64, order: &mut i64, value: bool) -> bool {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = ((*(order)).checked_mul(10).expect("checked arithmetic failed")).checked_add(1).expect("checked arithmetic failed"));
    return value;
}

fn observeResult(calls: &mut i64, order: &mut i64, marker: i64, value: bool) -> bool {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = ((*(order)).checked_mul(10).expect("checked arithmetic failed")).checked_add(marker).expect("checked arithmetic failed"));
    return value;
}

fn accept(value: bool) -> bool {
    return value;
}

fn choose(value: bool, subject_calls: &mut i64, result_calls: &mut i64, order: &mut i64) -> bool {
    return match (observeSubject(&mut *(subject_calls), &mut *(order), value)) {
         true => {
             (observeResult(&mut *(result_calls), &mut *(order), 2, true))
         },
         false => {
             (observeResult(&mut *(result_calls), &mut *(order), 3, false))
         },
         _ => {
             (observeResult(&mut *(result_calls), &mut *(order), 4, true))
         },
     };
}

fn main() {
    let mut __sn_match_result: bool = true;
    let mut subject_calls: i64 = 0;
    let mut result_calls: i64 = 0;
    let mut order: i64 = 0;
    let mut first: bool = match (observeSubject(&mut (subject_calls), &mut (order), true)) {
        true | false => {
            (observeResult(&mut (result_calls), &mut (order), 2, true))
        },
        true => {
            (observeResult(&mut (result_calls), &mut (order), 3, false))
        },
        _ => {
            (observeResult(&mut (result_calls), &mut (order), 4, false))
        },
    };
    let mut fallback: bool = match (observeSubject(&mut (subject_calls), &mut (order), false)) {
        true => {
            (observeResult(&mut (result_calls), &mut (order), 5, false))
        },
        _ => {
            (observeResult(&mut (result_calls), &mut (order), 6, true))
        },
    };
    let mut returned: bool = choose(false, &mut (subject_calls), &mut (result_calls), &mut (order));
    let mut argument: bool = match (true) {
        true => {
            (accept(observeResult(&mut (result_calls), &mut (order), 7, false)))
        },
        _ => {
            (accept(observeResult(&mut (result_calls), &mut (order), 8, true)))
        },
    };
    let mut nested: bool = match (true) {
        true => {
            (match (false) {
        true => {
            (observeResult(&mut (result_calls), &mut (order), 9, false))
        },
        _ => {
            (observeResult(&mut (result_calls), &mut (order), 4, true))
        },
    })
        },
        _ => {
            (observeResult(&mut (result_calls), &mut (order), 5, false))
        },
    };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", __sn_match_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", subject_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", result_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", first)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", fallback)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", returned)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", argument)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", nested)); __sn_interpolated });
}
