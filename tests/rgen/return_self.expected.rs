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
struct Point {
    x: i64,
    y: i64,
}

impl Point {
    fn snapshot(&self) -> Point {
        return self.clone();
    }
    fn shiftX(&mut self, amount: i64) -> Point {
        ((self).x = __sn_checked(((self).x).checked_add(amount), "Runtime error: integer overflow in addition")
);
        return self.clone();
    }
}

fn main() {
    let mut original: Point = Point { x: 1, y: 2 };
    let mut snapshot: Point = (original).snapshot();
    let mut shifted: Point = (original).shiftX(4);
    println!("{}", (original).x);
    println!("{}", (shifted).x);
    ((original).x = 9);
    ((original).y = 10);
    println!("{}", (snapshot).x);
    println!("{}", (snapshot).y);
    println!("{}", (shifted).x);
    println!("{}", (shifted).y);
    println!("{}", (original).x);
    println!("{}", (original).y);
}

