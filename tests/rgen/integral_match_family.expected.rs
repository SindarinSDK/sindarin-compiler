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

fn observeLong(calls: &mut i64, order: &mut i64, value: i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = __sn_checked((__sn_checked((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")
).checked_add(1), "Runtime error: integer overflow in addition")
);
    return value;
}

fn observeUint(calls: &mut i64, value: u64) -> u64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return value;
}

fn observeFloat(calls: &mut i64, order: &mut i64, marker: i64, value: f32) -> f32 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = __sn_checked((__sn_checked((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")
).checked_add(marker), "Runtime error: integer overflow in addition")
);
    return value;
}

fn observeDouble(calls: &mut i64, order: &mut i64, marker: i64, value: f64) -> f64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = __sn_checked((__sn_checked((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")
).checked_add(marker), "Runtime error: integer overflow in addition")
);
    return value;
}

fn main() {
    let mut __sn_match_subject: i64 = 41;
    let mut __sn_match_result: i64 = 42;
    let mut subject_calls: i64 = 0;
    let mut result_calls: i64 = 0;
    let mut order: i64 = 0;
    let mut selected: i64 = 0;
    match (observeLong(&mut (subject_calls), &mut (order), 2) as i64) {
        1 | 2 | 2 | 2 | 2 => {
            (selected = 10);
            (order = __sn_checked((__sn_checked((order).checked_mul(10), "Runtime error: integer overflow in multiplication")
).checked_add(2), "Runtime error: integer overflow in addition")
);
        },
        2 => {
            (selected = 20);
            (order = __sn_checked((__sn_checked((order).checked_mul(10), "Runtime error: integer overflow in multiplication")
).checked_add(8), "Runtime error: integer overflow in addition")
);
        },
        _ => {
            (selected = 30);
            (order = __sn_checked((__sn_checked((order).checked_mul(10), "Runtime error: integer overflow in multiplication")
).checked_add(9), "Runtime error: integer overflow in addition")
);
        },
    };
    let mut int32_selected: i64 = 0;
    let mut int32_value: i32 = (-7);
    match (int32_value as i32) {
        (-6) | (-7) | (-7) => {
            (int32_selected = 1);
        },
        _ => {
            (int32_selected = 2);
        },
    };
    let mut uint32_selected: i64 = 0;
    let mut uint32_value: u32 = 4;
    match (uint32_value as u32) {
        1 | 2 | 3 | 4 | 5 => {
            (uint32_selected = 1);
        },
        _ => {
            (uint32_selected = 2);
        },
    };
    let mut uint_selected: i64 = 0;
    let mut uint_value: u64 = 5;
    match (uint_value as u64) {
        1 | 2 | 3 | 4 | 5 => {
            (uint_selected = 1);
        },
        _ => {
            (uint_selected = 2);
        },
    };
    let mut byte_selected: i64 = 0;
    let mut byte_value: u8 = 255;
    match (byte_value as u8) {
        1 | 255 => {
            (byte_selected = 1);
        },
        _ => {
            (byte_selected = 2);
        },
    };
    let mut no_match: i64 = 7;
    match (9 as u8) {
        8 => {
            (no_match = 99);
        },
        _ => {},
    };
    let mut boundary_hits: i64 = 0;
    match ((-9223372036854775807) as i64) {
        (-9223372036854775807) => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    match (9223372036854775807 as i64) {
        9223372036854775807 => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    let mut int32_min: i32 = (-2147483647);
    { let __sn_rhs = 1; let __sn_place = &mut (int32_min); let __sn_next = __sn_checked((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
    match (int32_min as i32) {
        (-2147483648) => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    match (2147483647 as i32) {
        2147483647 => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    match (0 as u32) {
        0 => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    match (4294967295 as u32) {
        4294967295 => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    match (0 as u64) {
        0 => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    match (9223372036854775807 as u64) {
        9223372036854775807 => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    match (0 as u8) {
        0 => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    match (255 as u8) {
        255 => {
            { let __sn_place = &mut (boundary_hits); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        },
        _ => {},
    };
    let mut nested_statement: i64 = 0;
    match (1 as u32) {
        1 => {
            match (2 as u8) {
        2 => {
            (nested_statement = 12);
        },
        _ => {},
    };
        },
        _ => {},
    };
    let mut bool_result: bool = match (1 as i64) {
        1 => {
            (true)
        },
        _ => {
            (false)
        },
    };
    let mut int_result: i64 = match ((-2) as i32) {
        (-2) => {
            (20 as i64)
        },
        _ => {
            (0 as i64)
        },
    };
    let mut long_result: i64 = match (4294967295 as u32) {
        4294967295 => {
            ((-30) as i64)
        },
        _ => {
            (0 as i64)
        },
    };
    let mut int32_result: i32 = match (3 as u64) {
        3 => {
            ((-40) as i32)
        },
        _ => {
            (0 as i32)
        },
    };
    let mut uint32_result: u32 = match (4 as u8) {
        4 => {
            (50 as u32)
        },
        _ => {
            (0 as u32)
        },
    };
    let mut uint_result: u64 = match (5 as i64) {
        5 => {
            (60 as u64)
        },
        _ => {
            (0 as u64)
        },
    };
    let mut byte_result: u8 = match (6 as i32) {
        6 => {
            (70 as u8)
        },
        _ => {
            (0 as u8)
        },
    };
    let mut bool_subject_int_result: i64 = match (true) {
        true => {
            (80 as i64)
        },
        _ => {
            (0 as i64)
        },
    };
    let mut int_subject_bool_result: bool = match (1 as i64) {
        1 => {
            (true)
        },
        _ => {
            (false)
        },
    };
    let mut float_result: f32 = match (7 as u32) {
        7 => {
            (observeFloat(&mut (result_calls), &mut (order), 3, 6.25) as f32)
        },
        _ => {
            (observeFloat(&mut (result_calls), &mut (order), 8, 0.0) as f32)
        },
    };
    let mut double_result: f64 = match (observeUint(&mut (subject_calls), 8) as u64) {
        8 => {
            (observeDouble(&mut (result_calls), &mut (order), 4, 7.5) as f64)
        },
        _ => {
            (observeDouble(&mut (result_calls), &mut (order), 9, 0.0) as f64)
        },
    };
    let mut nested_value: i64 = match (1 as u8) {
        1 => {
            (match (2 as u32) {
        2 => {
            (77 as i64)
        },
        _ => {
            (0 as i64)
        },
    } as i64)
        },
        _ => {
            ((-1) as i64)
        },
    };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", __sn_match_subject)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", __sn_match_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", subject_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", result_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", selected)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", int32_selected)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", uint32_selected)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", uint_selected)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", byte_selected)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", no_match)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", boundary_hits)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", nested_statement)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", bool_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", int_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", long_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", int32_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", uint32_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", uint_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", byte_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", bool_subject_int_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", int_subject_bool_result)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (float_result == 6.25)
)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (double_result == 7.5)
)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", nested_value)); __sn_interpolated });
}

