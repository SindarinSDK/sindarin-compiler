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

#[derive(Clone, Copy, Debug, PartialEq)]
struct Pair {
    left: i64,
    right: i64,
}

fn sum(a: i64, b: i64) -> i64 {
    return __sn_checked(a.checked_add(b), "Runtime error: integer overflow in addition")
;
}

fn main() {
    let mut pair: Pair = Pair { left: 19, right: 23 };
    println!("{}", sum((pair).left, (pair).right));
}

