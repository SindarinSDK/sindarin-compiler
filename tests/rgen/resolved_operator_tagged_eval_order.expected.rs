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
    value: i64,
}

impl Point {
    fn op_lt(&self, other: Point) -> bool {
        return ((self).value < (other).value);
    }
}

fn marked(calls: &mut i64, order: &mut i64, marker: i64, value: i64) -> Point {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    return Point { value: value };
}

fn main() {
    let mut calls: i64 = 0;
    let mut order: i64 = 0;
    println!("{}", (marked(&mut (calls), &mut (order), 1, 1)).op_lt(marked(&mut (calls), &mut (order), 2, 2)));
    println!("{}", (marked(&mut (calls), &mut (order), 4, 4)).op_lt(marked(&mut (calls), &mut (order), 3, 3)));
    println!("{}", calls);
    println!("{}", order);
}
