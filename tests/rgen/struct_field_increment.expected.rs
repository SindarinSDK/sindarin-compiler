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
struct Counter {
    value: i64,
}

impl Counter {
    fn next(&mut self) -> i64 {
        return { let __sn_place = &mut ((self).value); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
    fn previous(&mut self) -> i64 {
        return { let __sn_place = &mut ((self).value); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    }
    fn advance(&mut self) -> i64 {
        return (self).next();
    }
}

fn main() {
    let mut counter: Counter = Counter { value: 5 };
    let mut beforeIncrement: i64 = (counter).advance();
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("increment="); __sn_interpolated.push_str(&format!("{}", beforeIncrement)); __sn_interpolated.push_str("->"); __sn_interpolated.push_str(&format!("{}", (counter).value)); __sn_interpolated });
    let mut beforeDecrement: i64 = (counter).previous();
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("decrement="); __sn_interpolated.push_str(&format!("{}", beforeDecrement)); __sn_interpolated.push_str("->"); __sn_interpolated.push_str(&format!("{}", (counter).value)); __sn_interpolated });
}
