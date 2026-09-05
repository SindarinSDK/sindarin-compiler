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
    fn longOps(value: &mut i64) -> i64 {
        let mut add: i64 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
        let mut sub: i64 = { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
        let mut mul: i64 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
        let mut div: i64 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_div((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
        let mut rem: i64 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_mod((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
        let mut old_inc: i64 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        let mut old_dec: i64 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
        return __sn_checked(__sn_checked(__sn_checked(__sn_checked(__sn_checked(__sn_checked(__sn_checked(add.checked_add(sub), "Runtime error: integer overflow in addition")
 .checked_add(mul), "Runtime error: integer overflow in addition")
 .checked_add(div), "Runtime error: integer overflow in addition")
 .checked_add(rem), "Runtime error: integer overflow in addition")
 .checked_add(old_inc), "Runtime error: integer overflow in addition")
 .checked_add(old_dec), "Runtime error: integer overflow in addition")
 .checked_add(*(value)), "Runtime error: integer overflow in addition")
;
    }
    fn intPostfix(&self, value: &mut i64) -> i64 {
        return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    }
    fn longPostfix(&self, value: &mut i64) -> i64 {
        return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    }
    fn intCompound(&self, value: &mut i64) -> i64 {
        return { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    }
    fn longCompound(&self, value: &mut i64) -> i64 {
        return { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
    }
}

fn oldThenIncrement(value: &mut i64) -> i64 {
    return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
}

fn intOps(value: &mut i64) -> i64 {
    let mut add: i64 = { let __sn_rhs = oldThenIncrement(&mut *(value)); let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    let mut sub: i64 = { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
    let mut mul: i64 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
    let mut div: i64 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_div((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    let mut rem: i64 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_mod((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    let mut old_inc: i64 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    let mut old_dec: i64 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    return __sn_checked(__sn_checked(__sn_checked(__sn_checked(__sn_checked(__sn_checked(__sn_checked(add.checked_add(sub), "Runtime error: integer overflow in addition")
 .checked_add(mul), "Runtime error: integer overflow in addition")
 .checked_add(div), "Runtime error: integer overflow in addition")
 .checked_add(rem), "Runtime error: integer overflow in addition")
 .checked_add(old_inc), "Runtime error: integer overflow in addition")
 .checked_add(old_dec), "Runtime error: integer overflow in addition")
 .checked_add(*(value)), "Runtime error: integer overflow in addition")
;
}

fn main() {
    let mut integer: i64 = 2;
    let mut long_value: i64 = 10;
    let mut ops: RefOps = RefOps {  };
    println!("{}", intOps(&mut (integer)));
    println!("{}", integer);
    println!("{}", RefOps::longOps(&mut (long_value)));
    println!("{}", long_value);
    println!("{}", (ops).intPostfix(&mut (integer)));
    println!("{}", integer);
    println!("{}", (ops).longPostfix(&mut (long_value)));
    println!("{}", long_value);
    println!("{}", (ops).intCompound(&mut (integer)));
    println!("{}", integer);
    println!("{}", (ops).longCompound(&mut (long_value)));
    println!("{}", long_value);
}

