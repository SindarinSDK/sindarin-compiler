#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn observeFloat(calls: &mut i64, order: &mut i64, marker: i64, value: f32) -> f32 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = ((*(order)).checked_mul(10).expect("checked arithmetic failed")).checked_add(marker).expect("checked arithmetic failed"));
    return value;
}

fn observeDouble(calls: &mut i64, order: &mut i64, marker: i64, value: f64) -> f64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = ((*(order)).checked_mul(10).expect("checked arithmetic failed")).checked_add(marker).expect("checked arithmetic failed"));
    return value;
}

fn observeInt(calls: &mut i64, order: &mut i64, marker: i64, value: i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = ((*(order)).checked_mul(10).expect("checked arithmetic failed")).checked_add(marker).expect("checked arithmetic failed"));
    return value;
}

fn chooseDouble(subject_calls: &mut i64, body_calls: &mut i64, order: &mut i64) -> f64 {
    return {
     let __sn_match_subject_0: f64 = observeDouble(&mut *(subject_calls), &mut *(order), 7, 9.5);
     if (__sn_match_subject_0 == 9.5) {
         (observeDouble(&mut *(body_calls), &mut *(order), 8, 42.25) as f64)
     }
     else {
         (observeDouble(&mut *(body_calls), &mut *(order), 9, 0.0) as f64)
     }
 };
}

fn main() {
    let mut __sn_match_subject: i64 = 41;
    let mut __sn_match_result: i64 = 42;
    let mut subject_calls: i64 = 0;
    let mut body_calls: i64 = 0;
    let mut order: i64 = 0;
    let mut selected: i64 = 0;
    {
    let __sn_match_subject_1: f32 = observeFloat(&mut (subject_calls), &mut (order), 1, 2.5);
    if (__sn_match_subject_1 == 1.0 || __sn_match_subject_1 == 2.5 || __sn_match_subject_1 == (-2.5)) {
        (selected = 10);
        observeInt(&mut (body_calls), &mut (order), 2, 0);
    }
    else if (__sn_match_subject_1 == 2.5) {
        (selected = 20);
        observeInt(&mut (body_calls), &mut (order), 8, 0);
    }
    else {
        (selected = 30);
        observeInt(&mut (body_calls), &mut (order), 9, 0);
    }
};
    {
    let __sn_match_subject_2: f64 = observeDouble(&mut (subject_calls), &mut (order), 3, (-4.5));
    if (__sn_match_subject_2 == 4.5 || __sn_match_subject_2 == (-4.5) || __sn_match_subject_2 == (-1.0)) {
        { let __sn_rhs = 20; let __sn_place = &mut (selected); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
        observeInt(&mut (body_calls), &mut (order), 4, 0);
    }
    else if (__sn_match_subject_2 == (-4.5)) {
        (selected = 200);
        observeInt(&mut (body_calls), &mut (order), 8, 0);
    }
    else {
        (selected = 300);
        observeInt(&mut (body_calls), &mut (order), 9, 0);
    }
};
    let mut nested_statement: i64 = 0;
    {
    let __sn_match_subject_4: f32 = 1.0;
    if (__sn_match_subject_4 == 1.0) {
        {
    let __sn_match_subject_3: f64 = 2.0;
    if (__sn_match_subject_3 == 2.0) {
        (nested_statement = 12);
    }
};
    }
};
    let mut nan_statement_hits: i64 = 0;
    {
    let __sn_match_subject_5: f32 = (0.0 / 0.0);
    if (__sn_match_subject_5 == 0.0) {
        { let __sn_place = &mut (nan_statement_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    }
};
    let mut zero_hits: i64 = 0;
    {
    let __sn_match_subject_6: f32 = (-0.0);
    if (__sn_match_subject_6 == 0.0) {
        { let __sn_place = &mut (zero_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    }
};
    {
    let __sn_match_subject_7: f64 = 0.0;
    if (__sn_match_subject_7 == (-0.0)) {
        { let __sn_place = &mut (zero_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_previous.checked_add(1).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_previous };
    }
};
    let mut first_value: i64 = {
    let __sn_match_subject_8: f32 = observeFloat(&mut (subject_calls), &mut (order), 5, (-2.5));
    if (__sn_match_subject_8 == (-2.5) || __sn_match_subject_8 == (-1.0)) {
        (observeInt(&mut (body_calls), &mut (order), 6, 100) as i64)
    }
    else if (__sn_match_subject_8 == (-2.5)) {
        (observeInt(&mut (body_calls), &mut (order), 8, 200) as i64)
    }
    else {
        (observeInt(&mut (body_calls), &mut (order), 9, 300) as i64)
    }
};
    let mut nan_value: bool = {
    let __sn_match_subject_9: f64 = (0.0 / 0.0);
    if (__sn_match_subject_9 == 0.0) {
        (false)
    }
    else {
        (true)
    }
};
    let mut float_value: f32 = {
    let __sn_match_subject_10: f32 = 7.25;
    if (__sn_match_subject_10 == 7.25) {
        ((-3.5) as f32)
    }
    else {
        (0.0 as f32)
    }
};
    let mut double_value: f64 = {
    let __sn_match_subject_11: f64 = (-6.5);
    if (__sn_match_subject_11 == (-6.5) || __sn_match_subject_11 == 1.0) {
        (6.75 as f64)
    }
    else {
        (0.0 as f64)
    }
};
    let mut returned: f64 = chooseDouble(&mut (subject_calls), &mut (body_calls), &mut (order));
    let mut nested_value: i64 = {
    let __sn_match_subject_13: f32 = (-1.0);
    if (__sn_match_subject_13 == (-1.0)) {
        ({
    let __sn_match_subject_12: f64 = 2.0;
    if (__sn_match_subject_12 == 2.0) {
        (77 as i64)
    }
    else {
        (0 as i64)
    }
} as i64)
    }
    else {
        ((-1) as i64)
    }
};
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", __sn_match_subject)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", __sn_match_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", subject_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", body_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", selected)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", nested_statement)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", nan_statement_hits)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", zero_hits)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", first_value)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", nan_value)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (float_value == (-3.5)))); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (double_value == 6.75))); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (returned == 42.25))); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", nested_value)); __sn_interpolated });
}
