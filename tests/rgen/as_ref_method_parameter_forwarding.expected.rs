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
struct RefOps {
}

impl RefOps {
    fn staticBump(value: &mut i64) {
        { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    }
    fn instanceBump(&self, value: &mut i64) {
        { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    }
}

fn forwardStatic(value: &mut i64) -> i64 {
    RefOps::staticBump(&mut *(value));
    return *(value);
}

fn forwardInstance(value: &mut i64) -> i64 {
    let mut ops: RefOps = RefOps {  };
    (ops).instanceBump(&mut *(value));
    return *(value);
}

fn main() {
    let mut staticValue: i64 = 1;
    let mut instanceValue: i64 = 1;
    println!("{}", forwardStatic(&mut (staticValue)));
    println!("{}", staticValue);
    println!("{}", forwardInstance(&mut (instanceValue)));
    println!("{}", instanceValue);
}

