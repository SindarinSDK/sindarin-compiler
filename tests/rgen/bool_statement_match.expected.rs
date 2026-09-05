#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn __sn_runtime_error_0(message: &'static str) -> ! {
    eprintln!("{}", message);
    std::process::exit(1);
}

fn __sn_checked_0<T>(value: Option<T>, message: &'static str) -> T {
    match value {
        Some(value) => value,
        None => __sn_runtime_error_0(message),
    }
}

fn __sn_checked_div_0<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked_0(value, if divisor_is_zero {
        "panic: Division by zero"
    } else {
        "Runtime error: integer overflow in division"
    })
}

fn __sn_checked_mod_0<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked_0(value, if divisor_is_zero {
        "panic: Modulo by zero"
    } else {
        "Runtime error: integer overflow in modulo"
    })
}

fn observeSubject(calls: &mut i64, order: &mut i64, value: bool) -> bool {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(1), "Runtime error: integer overflow in addition"));
    return value;
}

fn main() {
    let mut subject_calls: i64 = 0;
    let mut order: i64 = 0;
    let mut first: i64 = 0;
    match (observeSubject(&mut (subject_calls), &mut (order), true)) {
        true => {
            (order = __sn_checked_0((__sn_checked_0((order).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(2), "Runtime error: integer overflow in addition"));
            (first = 10);
        },
        true => {
            (order = __sn_checked_0((__sn_checked_0((order).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(3), "Runtime error: integer overflow in addition"));
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
