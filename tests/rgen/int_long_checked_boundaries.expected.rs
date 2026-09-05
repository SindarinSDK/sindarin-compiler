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

fn main() {
    let mut int_max_minus_one: i64 = 9223372036854775806;
    let mut int_one: i64 = 1;
    let mut int_max: i64 = __sn_checked(int_max_minus_one.checked_add(int_one), "Runtime error: integer overflow in addition")
;
    let mut int_min_base: i64 = (-9223372036854775807);
    let mut int_min: i64 = __sn_checked(int_min_base.checked_sub(int_one), "Runtime error: integer overflow in subtraction")
;
    let mut int_mul_base: i64 = (-4611686018427387904);
    let mut int_two: i64 = 2;
    let mut int_product: i64 = __sn_checked(int_mul_base.checked_mul(int_two), "Runtime error: integer overflow in multiplication")
;
    let mut int_quotient: i64 = { let __sn_left = int_min; let __sn_right = int_one; __sn_checked_div(__sn_left.checked_div(__sn_right), __sn_right == 0) }
;
    let mut int_remainder: i64 = { let __sn_left = int_min; let __sn_right = int_one; __sn_checked_mod(__sn_left.checked_rem(__sn_right), __sn_right == 0) }
;
    let mut long_max_minus_one: i64 = 9223372036854775806;
    let mut long_one: i64 = 1;
    let mut long_max: i64 = __sn_checked(long_max_minus_one.checked_add(long_one), "Runtime error: integer overflow in addition")
;
    let mut long_min_base: i64 = (-9223372036854775807);
    let mut long_min: i64 = __sn_checked(long_min_base.checked_sub(long_one), "Runtime error: integer overflow in subtraction")
;
    let mut long_mul_base: i64 = (-4611686018427387904);
    let mut long_two: i64 = 2;
    let mut long_product: i64 = __sn_checked(long_mul_base.checked_mul(long_two), "Runtime error: integer overflow in multiplication")
;
    let mut long_quotient: i64 = { let __sn_left = long_min; let __sn_right = long_one; __sn_checked_div(__sn_left.checked_div(__sn_right), __sn_right == 0) }
;
    let mut long_remainder: i64 = { let __sn_left = long_min; let __sn_right = long_one; __sn_checked_mod(__sn_left.checked_rem(__sn_right), __sn_right == 0) }
;
    if ((((((((((int_max == 9223372036854775807)
 && (__sn_checked(int_min.checked_add(int_one), "Runtime error: integer overflow in addition")
 == int_min_base)
)
 && (int_product == int_min)
)
 && (int_quotient == int_min)
)
 && (int_remainder == 0)
)
 && (long_max == 9223372036854775807)
)
 && (__sn_checked(long_min.checked_add(long_one), "Runtime error: integer overflow in addition")
 == long_min_base)
)
 && (long_product == long_min)
)
 && (long_quotient == long_min)
)
 && (long_remainder == 0)
)
 {
        println!("{}", "ok".to_string());
    } else {
        println!("{}", "wrong".to_string());
    }
}

