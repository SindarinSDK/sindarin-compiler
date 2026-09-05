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

fn rhs(calls: &mut i64) -> i64 {
    { let __sn_rhs = 1; let __sn_place = &mut (*(calls)); let __sn_next = __sn_checked((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    return 2;
}

fn main() {
    let mut values: Vec<i64> = vec![8];
    let mut calls: i64 = 0;
    for mut value in (values).iter().cloned() {
        let mut added: i64 = { let __sn_rhs = rhs(&mut (calls)); let __sn_place = &mut (value); let __sn_next = __sn_checked((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
        let mut divided: i64 = { let __sn_rhs = rhs(&mut (calls)); let __sn_place = &mut (value); let __sn_next = __sn_checked_div((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
        let mut before_inc: i64 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        let mut before_dec: i64 = { let __sn_place = &mut (value); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
        println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", added)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", divided)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", before_inc)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", before_dec)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", calls)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (values)[__sn_index((values).len(), 0)])); __sn_interpolated });
    }
}

