#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn __sn_runtime_error_1(message: &'static str) -> ! {
    eprintln!("{}", message);
    std::process::exit(1);
}

fn __sn_checked_1<T>(value: Option<T>, message: &'static str) -> T {
    match value {
        Some(value) => value,
        None => __sn_runtime_error_1(message),
    }
}

fn __sn_checked_div_1<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked_1(value, if divisor_is_zero {
        "panic: Division by zero"
    } else {
        "Runtime error: integer overflow in division"
    })
}

fn __sn_checked_mod_1<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked_1(value, if divisor_is_zero {
        "panic: Modulo by zero"
    } else {
        "Runtime error: integer overflow in modulo"
    })
}

fn __sn_checked(value: i64) -> i64 {
    return __sn_checked_1((value).checked_add(1), "Runtime error: integer overflow in addition");
}

fn __sn_checked_0(value: i64) -> i64 {
    return __sn_checked_1((value).checked_add(2), "Runtime error: integer overflow in addition");
}

fn __sn_checked_div_0(value: i64) -> i64 {
    return __sn_checked_1((value).checked_add(3), "Runtime error: integer overflow in addition");
}

fn __sn_checked_mod_0(value: i64) -> i64 {
    return __sn_checked_1((value).checked_add(4), "Runtime error: integer overflow in addition");
}

fn __sn_runtime_error_0(value: i64) -> i64 {
    return __sn_checked_1((value).checked_add(5), "Runtime error: integer overflow in addition");
}

fn main() {
    let mut result: i64 = __sn_checked(1);
    { let __sn_rhs = __sn_checked_0(1); let __sn_place = &mut (result); let __sn_next = __sn_checked_1((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = __sn_checked_div_0(1); let __sn_place = &mut (result); let __sn_next = __sn_checked_1((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = __sn_checked_mod_0(1); let __sn_place = &mut (result); let __sn_next = __sn_checked_1((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = __sn_runtime_error_0(1); let __sn_place = &mut (result); let __sn_next = __sn_checked_1((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    print!("{}", result);
    print!("{}", "\n".to_string());
}
