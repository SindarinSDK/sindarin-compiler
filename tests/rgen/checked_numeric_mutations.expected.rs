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
struct Numbers {
    i: i64,
    l: i64,
    i32: i32,
    u: u64,
    u32: u32,
    b: u8,
}

fn main() {
    let mut i: i64 = 20;
    let mut i_add: i64 = 4;
    let mut i_sub: i64 = 2;
    let mut i_mul: i64 = 2;
    let mut i_div: i64 = 2;
    let mut i_mod: i64 = 5;
    let mut l: i64 = 20;
    let mut l_add: i64 = 4;
    let mut l_sub: i64 = 2;
    let mut l_mul: i64 = 2;
    let mut l_div: i64 = 2;
    let mut l_mod: i64 = 5;
    let mut i32: i32 = 20;
    let mut i32_add: i32 = 4;
    let mut i32_sub: i32 = 2;
    let mut i32_mul: i32 = 2;
    let mut i32_div: i32 = 2;
    let mut i32_mod: i32 = 5;
    let mut u: u64 = 20;
    let mut u_add: u64 = 4;
    let mut u_sub: u64 = 2;
    let mut u_mul: u64 = 2;
    let mut u_div: u64 = 2;
    let mut u_mod: u64 = 5;
    let mut u32: u32 = 20;
    let mut u32_add: u32 = 4;
    let mut u32_sub: u32 = 2;
    let mut u32_mul: u32 = 2;
    let mut u32_div: u32 = 2;
    let mut u32_mod: u32 = 5;
    let mut b: u8 = 20;
    let mut b_add: u8 = 4;
    let mut b_sub: u8 = 2;
    let mut b_mul: u8 = 2;
    let mut b_div: u8 = 2;
    let mut b_mod: u8 = 5;
    let mut fields: Numbers = Numbers { i: 20, l: 20, i32: 20, u: 20, u32: 20, b: 20 };
    { let __sn_rhs = i_add; let __sn_place = &mut (i); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i_sub; let __sn_place = &mut (i); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i_mul; let __sn_place = &mut (i); let __sn_next = __sn_checked_0((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i_div; let __sn_place = &mut (i); let __sn_next = __sn_checked_div_0((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i_mod; let __sn_place = &mut (i); let __sn_next = __sn_checked_mod_0((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i_add; let __sn_place = &mut ((fields).i); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i_sub; let __sn_place = &mut ((fields).i); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i_mul; let __sn_place = &mut ((fields).i); let __sn_next = __sn_checked_0((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i_div; let __sn_place = &mut ((fields).i); let __sn_next = __sn_checked_div_0((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i_mod; let __sn_place = &mut ((fields).i); let __sn_next = __sn_checked_mod_0((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = l_add; let __sn_place = &mut (l); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = l_sub; let __sn_place = &mut (l); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = l_mul; let __sn_place = &mut (l); let __sn_next = __sn_checked_0((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = l_div; let __sn_place = &mut (l); let __sn_next = __sn_checked_div_0((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = l_mod; let __sn_place = &mut (l); let __sn_next = __sn_checked_mod_0((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = l_add; let __sn_place = &mut ((fields).l); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = l_sub; let __sn_place = &mut ((fields).l); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = l_mul; let __sn_place = &mut ((fields).l); let __sn_next = __sn_checked_0((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = l_div; let __sn_place = &mut ((fields).l); let __sn_next = __sn_checked_div_0((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = l_mod; let __sn_place = &mut ((fields).l); let __sn_next = __sn_checked_mod_0((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i32_add; let __sn_place = &mut (i32); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i32_sub; let __sn_place = &mut (i32); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i32_mul; let __sn_place = &mut (i32); let __sn_next = __sn_checked_0((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i32_div; let __sn_place = &mut (i32); let __sn_next = __sn_checked_div_0((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i32_mod; let __sn_place = &mut (i32); let __sn_next = __sn_checked_mod_0((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i32_add; let __sn_place = &mut ((fields).i32); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i32_sub; let __sn_place = &mut ((fields).i32); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i32_mul; let __sn_place = &mut ((fields).i32); let __sn_next = __sn_checked_0((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i32_div; let __sn_place = &mut ((fields).i32); let __sn_next = __sn_checked_div_0((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = i32_mod; let __sn_place = &mut ((fields).i32); let __sn_next = __sn_checked_mod_0((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u_add; let __sn_place = &mut (u); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u_sub; let __sn_place = &mut (u); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u_mul; let __sn_place = &mut (u); let __sn_next = __sn_checked_0((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u_div; let __sn_place = &mut (u); let __sn_next = __sn_checked_div_0((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u_mod; let __sn_place = &mut (u); let __sn_next = __sn_checked_mod_0((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u_add; let __sn_place = &mut ((fields).u); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u_sub; let __sn_place = &mut ((fields).u); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u_mul; let __sn_place = &mut ((fields).u); let __sn_next = __sn_checked_0((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u_div; let __sn_place = &mut ((fields).u); let __sn_next = __sn_checked_div_0((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u_mod; let __sn_place = &mut ((fields).u); let __sn_next = __sn_checked_mod_0((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u32_add; let __sn_place = &mut (u32); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u32_sub; let __sn_place = &mut (u32); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u32_mul; let __sn_place = &mut (u32); let __sn_next = __sn_checked_0((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u32_div; let __sn_place = &mut (u32); let __sn_next = __sn_checked_div_0((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u32_mod; let __sn_place = &mut (u32); let __sn_next = __sn_checked_mod_0((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u32_add; let __sn_place = &mut ((fields).u32); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u32_sub; let __sn_place = &mut ((fields).u32); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u32_mul; let __sn_place = &mut ((fields).u32); let __sn_next = __sn_checked_0((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u32_div; let __sn_place = &mut ((fields).u32); let __sn_next = __sn_checked_div_0((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = u32_mod; let __sn_place = &mut ((fields).u32); let __sn_next = __sn_checked_mod_0((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = b_add; let __sn_place = &mut (b); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = b_sub; let __sn_place = &mut (b); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = b_mul; let __sn_place = &mut (b); let __sn_next = __sn_checked_0((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = b_div; let __sn_place = &mut (b); let __sn_next = __sn_checked_div_0((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = b_mod; let __sn_place = &mut (b); let __sn_next = __sn_checked_mod_0((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = b_add; let __sn_place = &mut ((fields).b); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = b_sub; let __sn_place = &mut ((fields).b); let __sn_next = __sn_checked_0((*__sn_place).checked_sub(__sn_rhs), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = b_mul; let __sn_place = &mut ((fields).b); let __sn_next = __sn_checked_0((*__sn_place).checked_mul(__sn_rhs), "Runtime error: integer overflow in multiplication"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = b_div; let __sn_place = &mut ((fields).b); let __sn_next = __sn_checked_div_0((*__sn_place).checked_div(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = b_mod; let __sn_place = &mut ((fields).b); let __sn_next = __sn_checked_mod_0((*__sn_place).checked_rem(__sn_rhs), __sn_rhs == 0); *__sn_place = __sn_next; __sn_next };
    let mut i_before: i64 = { let __sn_place = &mut (i); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut (i); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    let mut field_i_before: i64 = { let __sn_place = &mut ((fields).i); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut ((fields).i); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    let mut l_before: i64 = { let __sn_place = &mut (l); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut (l); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    let mut field_l_before: i64 = { let __sn_place = &mut ((fields).l); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut ((fields).l); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    let mut i32_before: i32 = { let __sn_place = &mut (i32); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut (i32); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    let mut field_i32_before: i32 = { let __sn_place = &mut ((fields).i32); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut ((fields).i32); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    let mut u_before: u64 = { let __sn_place = &mut (u); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut (u); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    let mut field_u_before: u64 = { let __sn_place = &mut ((fields).u); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut ((fields).u); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    let mut u32_before: u32 = { let __sn_place = &mut (u32); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut (u32); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    let mut field_u32_before: u32 = { let __sn_place = &mut ((fields).u32); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut ((fields).u32); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    let mut b_before: u8 = { let __sn_place = &mut (b); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut (b); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    let mut field_b_before: u8 = { let __sn_place = &mut ((fields).b); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    { let __sn_place = &mut ((fields).b); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", i_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", i)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", field_i_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (fields).i)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", l_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", l)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", field_l_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (fields).l)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", i32_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", i32)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", field_i32_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (fields).i32)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", u_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", u)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", field_u_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (fields).u)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", u32_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", u32)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", field_u32_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (fields).u32)); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", b_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", b)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", field_b_before)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (fields).b)); __sn_interpolated });
}
