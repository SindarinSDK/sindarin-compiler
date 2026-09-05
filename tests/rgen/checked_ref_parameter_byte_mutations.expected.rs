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
struct Box {
    value: u8,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct Wrapper {
    r#box: Box,
}
#[derive(Clone, Copy, Debug, PartialEq)]
struct RefOps {
}

impl RefOps {
    fn staticOps(value: &mut u8) -> u8 {
        let mut add: u8 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
        let mut sub: u8 = { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
        let mut mul: u8 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
        let mut div: u8 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_div((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
        let mut rem: u8 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_mod((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
        let mut old_inc: u8 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        let mut old_dec: u8 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
        return __sn_checked(__sn_checked(__sn_checked(__sn_checked(__sn_checked(__sn_checked(__sn_checked(add.checked_add(sub), "Runtime error: integer overflow in addition")
 .checked_add(mul), "Runtime error: integer overflow in addition")
 .checked_add(div), "Runtime error: integer overflow in addition")
 .checked_add(rem), "Runtime error: integer overflow in addition")
 .checked_add(old_inc), "Runtime error: integer overflow in addition")
 .checked_add(old_dec), "Runtime error: integer overflow in addition")
 .checked_add(*(value)), "Runtime error: integer overflow in addition")
;
    }
    fn staticBump(value: &mut u8) {
        { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    }
    fn instancePostfix(&self, value: &mut u8) -> u8 {
        return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    }
    fn instanceBump(&self, value: &mut u8) {
        { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    }
}

fn oldThenIncrement(value: &mut u8) -> u8 {
    return { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
}

fn freeOps(value: &mut u8) -> u8 {
    let mut add: u8 = { let __sn_rhs = oldThenIncrement(&mut *(value)); let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    let mut sub: u8 = { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
    let mut mul: u8 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
    let mut div: u8 = { let __sn_rhs = 2; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_div((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    let mut rem: u8 = { let __sn_rhs = 3; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked_mod((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    let mut old_inc: u8 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    let mut old_dec: u8 = { let __sn_place = &mut (*(value)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    return __sn_checked(__sn_checked(__sn_checked(__sn_checked(__sn_checked(__sn_checked(__sn_checked(add.checked_add(sub), "Runtime error: integer overflow in addition")
 .checked_add(mul), "Runtime error: integer overflow in addition")
 .checked_add(div), "Runtime error: integer overflow in addition")
 .checked_add(rem), "Runtime error: integer overflow in addition")
 .checked_add(old_inc), "Runtime error: integer overflow in addition")
 .checked_add(old_dec), "Runtime error: integer overflow in addition")
 .checked_add(*(value)), "Runtime error: integer overflow in addition")
;
}

fn forwardStatic(value: &mut u8) -> u8 {
    RefOps::staticBump(&mut *(value));
    return *(value);
}

fn forwardInstance(value: &mut u8) -> u8 {
    let mut ops: RefOps = RefOps {  };
    (ops).instanceBump(&mut *(value));
    return *(value);
}

fn maxBoundary(value: &mut u8) -> u8 {
    return { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
}

fn minBoundary(value: &mut u8) -> u8 {
    return { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
}

fn read(value: &mut u8) -> u8 {
    return *(value);
}

fn freeBump(value: &mut u8) {
    { let __sn_rhs = 1; let __sn_place = &mut (*(value)); let __sn_next = __sn_checked((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
}

fn main() {
    let mut free_value: u8 = 2;
    let mut static_value: u8 = 10;
    let mut postfix_value: u8 = 5;
    let mut forwarded_static: u8 = 1;
    let mut forwarded_instance: u8 = 1;
    let mut maximum: u8 = 254;
    let mut minimum: u8 = 1;
    let mut direct: Box = Box { value: 1 };
    let mut nested: Wrapper = Wrapper { r#box: Box { value: 10 } };
    let mut ops: RefOps = RefOps {  };
    freeBump(&mut ((direct).value));
    freeBump(&mut (((nested).r#box).value));
    RefOps::staticBump(&mut ((direct).value));
    RefOps::staticBump(&mut (((nested).r#box).value));
    (ops).instanceBump(&mut ((direct).value));
    (ops).instanceBump(&mut (((nested).r#box).value));
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", freeOps(&mut (free_value)))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", free_value)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", RefOps::staticOps(&mut (static_value)))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", static_value)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (ops).instancePostfix(&mut (postfix_value)))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", postfix_value)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", forwardStatic(&mut (forwarded_static)))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", forwarded_static)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", forwardInstance(&mut (forwarded_instance)))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", forwarded_instance)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", maxBoundary(&mut (maximum)))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", minBoundary(&mut (minimum)))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", read(&mut ((direct).value)))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (direct).value)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", ((nested).r#box).value)); __sn_interpolated });
}

