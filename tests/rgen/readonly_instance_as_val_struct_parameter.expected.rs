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

#[derive(Clone, Copy, Debug, PartialEq)]
struct Point {
    x: i64,
}

impl Point {
    fn offset(&self, mut other: Point) -> i64 {
        let mut replacement: Point = Point { x: __sn_checked_0(((other).x).checked_add((self).x), "Runtime error: integer overflow in addition") };
        (other = replacement);
        return (other).x;
    }
}

fn main() {
    let mut point: Point = Point { x: 1 };
    let mut other: Point = Point { x: 2 };
    println!("{}", (point).offset(other));
    println!("{}", (other).x);
    println!("{}", (point).x);
    println!("{}", (point).offset(point));
    println!("{}", (point).x);
}
