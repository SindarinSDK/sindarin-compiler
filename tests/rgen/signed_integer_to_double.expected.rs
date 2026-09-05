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

fn bumpAndReturn(counter: &mut i64) -> i64 {
    { let __sn_rhs = 1; let __sn_place = &mut (*(counter)); let __sn_next = __sn_checked((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    return (-7);
}

fn main() {
    let mut zero: i64 = 0;
    let mut negative: i64 = (-7);
    let mut positive: i64 = 42;
    let mut long_value: i64 = 42;
    let mut exact_long: i64 = 9007199254740991;
    let mut counter: i64 = 0;
    let mut called: f64 = (bumpAndReturn(&mut (counter)) as f64);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((((((zero as f64) == 0.0)
 && ((negative as f64) == (-7.0))
)
 && ((positive as f64) == 42.0)
)
 && ((long_value as f64) == 42.0)
)
 && ((exact_long as f64) == 9007199254740991.0)
)
)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", ((counter == 1)
 && (called == (-7.0))
)
)); __sn_interpolated });
}

