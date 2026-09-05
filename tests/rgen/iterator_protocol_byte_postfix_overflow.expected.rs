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
struct ByteIterator {
    value: u8,
    remaining: i64,
}

impl ByteIterator {
    fn iter(&self) -> ByteIterator {
        return self.clone();
    }
    fn hasNext(&self) -> bool {
        return ((self).remaining > 0);
    }
    fn next(&mut self) -> u8 {
        { let __sn_rhs = 1; let __sn_place = &mut ((self).remaining); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
        return (self).value;
    }
}

fn main() {
    let mut source: ByteIterator = ByteIterator { value: 255, remaining: 1 };
    {
    let mut __sn_iter_0 = (source).iter();
    while __sn_iter_0.hasNext() {
        let mut value = __sn_iter_0.next();
        let mut previous: u8 = { let __sn_byte_place = &mut (value); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous };
        { let __sn_byte_place = &mut (previous); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous };
    }
}
}
