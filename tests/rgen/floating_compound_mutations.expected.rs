#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn __sn_index(length: usize, index: i64) -> usize {
    let resolved = if index < 0 { length as i64 + index } else { index };
    if resolved < 0 || resolved >= length as i64 {
        panic!("array index out of bounds: {index}");
    }
    resolved as usize
}

fn __sn_insert_index(length: usize, index: i64) -> usize {
    let resolved = if index < 0 { length as i64 + index } else { index };
    if resolved < 0 || resolved > length as i64 {
        panic!("array insert index out of bounds: {index}");
    }
    resolved as usize
}

fn __sn_array_size(size: i64) -> usize {
    if size < 0 {
        panic!("array size cannot be negative: {size}");
    }
    size as usize
}

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

#[derive(Clone, Copy, Debug, PartialEq)]
struct FloatingValues {
    single: f32,
    precise: f64,
}

impl FloatingValues {
    fn halve(&mut self) -> f64 {
        return { let (__sn_rhs, __sn_place) = (2.0, &mut ((self).precise)); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
    }
}

fn rhsFloat(calls: &mut i64) -> f32 {
    { let __sn_rhs = 1; let __sn_place = &mut (*(calls)); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    return 2.0;
}

fn main() {
    let mut single: f32 = 16.0;
    let mut single_add: f32 = { let (__sn_rhs, __sn_place) = (4.0, &mut (single)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut single_subtract: f32 = { let (__sn_rhs, __sn_place) = (2.0, &mut (single)); let __sn_next = *__sn_place - __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut single_multiply: f32 = { let (__sn_rhs, __sn_place) = (0.5, &mut (single)); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut single_divide: f32 = { let (__sn_rhs, __sn_place) = (3.0, &mut (single)); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
    println!("{}", (((((single_add == 20.0) && (single_subtract == 18.0)) && (single_multiply == 9.0)) && (single_divide == 3.0)) && (single == 3.0)));
    let mut precise: f64 = 32.0;
    let mut precise_add: f64 = { let (__sn_rhs, __sn_place) = (8.0, &mut (precise)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut precise_subtract: f64 = { let (__sn_rhs, __sn_place) = (4.0, &mut (precise)); let __sn_next = *__sn_place - __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut precise_multiply: f64 = { let (__sn_rhs, __sn_place) = (0.5, &mut (precise)); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut precise_divide: f64 = { let (__sn_rhs, __sn_place) = (3.0, &mut (precise)); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
    println!("{}", (((((precise_add == 40.0) && (precise_subtract == 36.0)) && (precise_multiply == 18.0)) && (precise_divide == 6.0)) && (precise == 6.0)));
    let mut fields: FloatingValues = FloatingValues { single: 2.0, precise: 8.0 };
    let mut field_single: f32 = { let (__sn_rhs, __sn_place) = (4.0, &mut ((fields).single)); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut field_precise: f64 = { let (__sn_rhs, __sn_place) = (2.0, &mut ((fields).precise)); let __sn_next = *__sn_place - __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut self_result: f64 = (fields).halve();
    println!("{}", (((((field_single == 8.0) && ((fields).single == 8.0)) && (field_precise == 6.0)) && (self_result == 3.0)) && ((fields).precise == 3.0)));
    let mut calls: i64 = 0;
    let mut __sn_rhs: f32 = 8.0;
    let mut ordered_result: f32 = { let (__sn_rhs, __sn_place) = (rhsFloat(&mut (calls)), &mut (__sn_rhs)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
    println!("{}", (((ordered_result == 10.0) && (__sn_rhs == 10.0)) && (calls == 1)));
    let mut __sn_place: f64 = 8.0;
    let mut place_result: f64 = { let (__sn_rhs, __sn_place) = (2.0, &mut (__sn_place)); let __sn_next = *__sn_place - __sn_rhs; *__sn_place = __sn_next; __sn_next };
    let mut __sn_next: f32 = 3.0;
    let mut next_result: f32 = { let (__sn_rhs, __sn_place) = (2.0, &mut (__sn_next)); let __sn_next = *__sn_place * __sn_rhs; *__sn_place = __sn_next; __sn_next };
    println!("{}", ((((place_result == 6.0) && (__sn_place == 6.0)) && (next_result == 6.0)) && (__sn_next == 6.0)));
    let mut singles: Vec<f32> = vec![8.0, 8.0];
    let mut single_index: i64 = 0;
    for mut value in (singles).iter().cloned() {
        let mut result: f32 = { let (__sn_rhs, __sn_place) = (2.0, &mut (value)); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
        println!("{}", (((result == 4.0) && (value == 4.0)) && ((singles)[__sn_index((singles).len(), single_index)] == 8.0)));
        { let __sn_rhs = 1; let __sn_place = &mut (single_index); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    }
    let mut doubles: Vec<f64> = vec![16.0];
    for mut value in (doubles).iter().cloned() {
        let mut result: f64 = { let (__sn_rhs, __sn_place) = (4.0, &mut (value)); let __sn_next = *__sn_place + __sn_rhs; *__sn_place = __sn_next; __sn_next };
        println!("{}", (((result == 20.0) && (value == 20.0)) && ((doubles)[__sn_index((doubles).len(), 0)] == 16.0)));
    }
    let mut zero: f64 = 0.0;
    let mut infinity: f64 = 1.0;
    let mut infinity_result: f64 = { let (__sn_rhs, __sn_place) = (zero, &mut (infinity)); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
    println!("{}", ((infinity_result == infinity) && (infinity > 1.0)));
}
