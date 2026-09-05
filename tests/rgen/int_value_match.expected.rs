#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn __sn_runtime_error(message: &'static str) -> ! {
    eprintln!("{}", message);
    std::process::exit(1);
}

fn __sn_checked<T>(value: Option<T>, message: &'static str) -> T {
    match value {
        Some(value) => value,
        None => __sn_runtime_error(message),
    }
}

fn __sn_checked_div<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked(value, if divisor_is_zero {
        "panic: Division by zero"
    } else {
        "Runtime error: integer overflow in division"
    })
}

fn __sn_checked_mod<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked(value, if divisor_is_zero {
        "panic: Modulo by zero"
    } else {
        "Runtime error: integer overflow in modulo"
    })
}

fn observeSubject(calls: &mut i64, value: i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return value;
}

fn observeResult(calls: &mut i64, value: i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return value;
}

fn choose(value: i64, subject_calls: &mut i64, result_calls: &mut i64) -> i64 {
    return match (observeSubject(&mut *(subject_calls), value) as i64) {
         1 | 2 => {
             (observeResult(&mut *(result_calls), 10) as i64)
         },
         2 | 3 => {
             (observeResult(&mut *(result_calls), 20) as i64)
         },
         _ => {
             (observeResult(&mut *(result_calls), 30) as i64)
         },
     };
}

fn main() {
    let mut __sn_match_result: i64 = 41;
    let mut subject_calls: i64 = 0;
    let mut result_calls: i64 = 0;
    let mut first: i64 = match (observeSubject(&mut (subject_calls), 2) as i64) {
        1 | 2 => {
            (observeResult(&mut (result_calls), 10) as i64)
        },
        2 | 3 => {
            (observeResult(&mut (result_calls), 20) as i64)
        },
        _ => {
            (observeResult(&mut (result_calls), 30) as i64)
        },
    };
    let mut fallback: i64 = match (observeSubject(&mut (subject_calls), 99) as i64) {
        1 | 2 => {
            (observeResult(&mut (result_calls), 10) as i64)
        },
        2 | 3 => {
            (observeResult(&mut (result_calls), 20) as i64)
        },
        _ => {
            (observeResult(&mut (result_calls), 30) as i64)
        },
    };
    let mut returned: i64 = choose(3, &mut (subject_calls), &mut (result_calls));
    let mut nested: i64 = match ((-9223372036854775807) as i64) {
        (-9223372036854775807) | 9223372036854775807 => {
            (match (9223372036854775807 as i64) {
        (-9223372036854775807) => {
            (observeResult(&mut (result_calls), 40) as i64)
        },
        9223372036854775807 => {
            (observeResult(&mut (result_calls), 50) as i64)
        },
        _ => {
            (observeResult(&mut (result_calls), 60) as i64)
        },
    } as i64)
        },
        _ => {
            (observeResult(&mut (result_calls), 70) as i64)
        },
    };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", __sn_match_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", subject_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", result_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", first)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", fallback)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", returned)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", nested)); __sn_interpolated });
}

