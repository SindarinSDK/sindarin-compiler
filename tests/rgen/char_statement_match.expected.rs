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

fn observeSubject(calls: &mut i64, order: &mut i64, value: char) -> char {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(1), "Runtime error: integer overflow in addition"));
    return value;
}

fn main() {
    let mut subject_calls: i64 = 0;
    let mut order: i64 = 0;
    let mut selected: i64 = 0;
    match (observeSubject(&mut (subject_calls), &mut (order), '\u{62}') as char) {
        '\u{61}' => {
            (selected = 10);
            (order = __sn_checked_0((__sn_checked_0((order).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(2), "Runtime error: integer overflow in addition"));
        },
        '\u{62}' => {
            (selected = 20);
            (order = __sn_checked_0((__sn_checked_0((order).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(3), "Runtime error: integer overflow in addition"));
        },
        _ => {
            (selected = 30);
            (order = __sn_checked_0((__sn_checked_0((order).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(4), "Runtime error: integer overflow in addition"));
        },
    };
    let mut fallback: i64 = 0;
    match ('\u{7a}' as char) {
        '\u{61}' => {
            (fallback = 1);
        },
        _ => {
            (fallback = 7);
        },
    };
    let mut unchanged: i64 = 11;
    match ('\u{7a}' as char) {
        '\u{61}' => {
            (unchanged = 99);
        },
        _ => {},
    };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", subject_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", selected)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", fallback)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", unchanged)); __sn_interpolated });
}
