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

fn increment(point: &mut Point) {
    ((point).x = __sn_checked_0(((point).x).checked_add(1), "Runtime error: integer overflow in addition"));
}

fn increment_twice(point: &mut Point) {
    increment(&mut *(point))
;
    increment(&mut *(point))
;
}

fn main() {
    let mut point: Point = Point { x: 1 };
    increment_twice(&mut (point))
;
    println!("{}", (point).x)
;
}
