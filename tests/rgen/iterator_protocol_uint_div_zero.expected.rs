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
struct UintIterator {
    value: u64,
    remaining: i64,
}

impl UintIterator {
    fn iter(&self) -> UintIterator {
        return self.clone();
    }
    fn hasNext(&self) -> bool {
        return ((self).remaining > 0)
;
    }
    fn next(&mut self) -> u64 {
        { let __sn_rhs = 1; let __sn_place = &mut ((self).remaining); let __sn_next = __sn_checked((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
        return (self).value;
    }
}

fn main() {
    let mut source: UintIterator = UintIterator { value: 1, remaining: 1 };
    let mut zero: u64 = 0;
    {
    let mut __sn_iter_0 = (source).iter();
    while __sn_iter_0.hasNext() {
        let mut value = __sn_iter_0.next();
        { let __sn_rhs = zero; let __sn_place = &mut (value); let __sn_next = __sn_checked_div((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
        let mut checked: u64 = 1;
        { let __sn_rhs = zero; let __sn_place = &mut (checked); let __sn_next = __sn_checked_div((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    }
}
}

