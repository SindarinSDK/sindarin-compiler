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

struct __SnClosure<F: ?Sized>(std::rc::Rc<F>);
impl<F: ?Sized> Clone for __SnClosure<F> {
    fn clone(&self) -> Self { Self(self.0.clone()) }
}
impl<F: ?Sized> std::fmt::Debug for __SnClosure<F> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("<function>")
    }
}
impl<F: ?Sized> PartialEq for __SnClosure<F> {
    fn eq(&self, other: &Self) -> bool { std::rc::Rc::ptr_eq(&self.0, &other.0) }
}
fn main() {
    let mut add32: u32 = 4294967295;
    let mut sub32: u32 = 0;
    let mut mul32: u32 = 2147483648;
    let mut div32: u32 = 4294967295;
    let mut mod32: u32 = 4294967295;
    let mut inc32: u32 = 4294967295;
    let mut dec32: u32 = 0;
    let mut max: u64 = { let __sn_byte_operand: u64 = 0; !__sn_byte_operand };
    let mut high: u64 = { let (__sn_byte_left, __sn_byte_right): (u64, u64) = ((max / 2), 1); __sn_byte_left.wrapping_add(__sn_byte_right) };
    let mut add: u64 = max;
    let mut sub: u64 = 0;
    let mut mul: u64 = high;
    let mut div: u64 = max;
    let mut r#mod: u64 = max;
    let mut inc: u64 = max;
    let mut dec: u64 = 0;
    let mut boundaries: __SnClosure<dyn Fn() -> bool> = { let (add32, sub32, mul32, div32, mod32, inc32, dec32, add, sub, mul, div, r#mod, inc, dec, max, ) = (add32.clone(), sub32.clone(), mul32.clone(), div32.clone(), mod32.clone(), inc32.clone(), dec32.clone(), add.clone(), sub.clone(), mul.clone(), div.clone(), r#mod.clone(), inc.clone(), dec.clone(), max.clone(), ); self::__SnClosure::<dyn Fn() -> bool>(std::rc::Rc::new(move || -> bool { let mut add32 = add32; let mut sub32 = sub32; let mut mul32 = mul32; let mut div32 = div32; let mut mod32 = mod32; let mut inc32 = inc32; let mut dec32 = dec32; let mut add = add; let mut sub = sub; let mut mul = mul; let mut div = div; let mut r#mod = r#mod; let mut inc = inc; let mut dec = dec; { let (__sn_byte_rhs, __sn_byte_place): (u32, &mut u32) = (1, &mut (add32)); let __sn_byte_next = (*__sn_byte_place).wrapping_add(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };{ let (__sn_byte_rhs, __sn_byte_place): (u32, &mut u32) = (1, &mut (sub32)); let __sn_byte_next = (*__sn_byte_place).wrapping_sub(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };{ let (__sn_byte_rhs, __sn_byte_place): (u32, &mut u32) = (2, &mut (mul32)); let __sn_byte_next = (*__sn_byte_place).wrapping_mul(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };{ let (__sn_byte_rhs, __sn_byte_place): (u32, &mut u32) = (2, &mut (div32)); let __sn_byte_next = *__sn_byte_place / __sn_byte_rhs; *__sn_byte_place = __sn_byte_next; __sn_byte_next };{ let (__sn_byte_rhs, __sn_byte_place): (u32, &mut u32) = (2, &mut (mod32)); let __sn_byte_next = *__sn_byte_place % __sn_byte_rhs; *__sn_byte_place = __sn_byte_next; __sn_byte_next };let mut inc32Before: u32 = { let __sn_byte_place = &mut (inc32); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous };let mut dec32Before: u32 = { let __sn_byte_place = &mut (dec32); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_sub(1); __sn_byte_previous };{ let (__sn_byte_rhs, __sn_byte_place): (u64, &mut u64) = (1, &mut (add)); let __sn_byte_next = (*__sn_byte_place).wrapping_add(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };{ let (__sn_byte_rhs, __sn_byte_place): (u64, &mut u64) = (1, &mut (sub)); let __sn_byte_next = (*__sn_byte_place).wrapping_sub(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };{ let (__sn_byte_rhs, __sn_byte_place): (u64, &mut u64) = (2, &mut (mul)); let __sn_byte_next = (*__sn_byte_place).wrapping_mul(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };{ let (__sn_byte_rhs, __sn_byte_place): (u64, &mut u64) = (2, &mut (div)); let __sn_byte_next = *__sn_byte_place / __sn_byte_rhs; *__sn_byte_place = __sn_byte_next; __sn_byte_next };{ let (__sn_byte_rhs, __sn_byte_place): (u64, &mut u64) = (2, &mut (r#mod)); let __sn_byte_next = *__sn_byte_place % __sn_byte_rhs; *__sn_byte_place = __sn_byte_next; __sn_byte_next };let mut incBefore: u64 = { let __sn_byte_place = &mut (inc); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous };let mut decBefore: u64 = { let __sn_byte_place = &mut (dec); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_sub(1); __sn_byte_previous };let mut result: bool = ((add32.clone() == 0) && (sub32.clone() == 4294967295));(result = (((result && (mul32.clone() == 0)) && (div32.clone() == 2147483647)) && (mod32.clone() == 1)));(result = ((result && (inc32Before == 4294967295)) && (inc32.clone() == 0)));(result = ((result && (dec32Before == 0)) && (dec32.clone() == 4294967295)));(result = ((result && (add.clone() == 0)) && (sub.clone() == max.clone())));(result = (((result && (mul.clone() == 0)) && (div.clone() == (max.clone() / 2))) && (r#mod.clone() == 1)));(result = ((result && (incBefore == max.clone())) && (inc.clone() == 0)));(result = ((result && (decBefore == 0)) && (dec.clone() == max.clone())));return result;})) }
;
    println!("{}", ((boundaries.clone()).0)());
    println!("{}", ((boundaries.clone()).0)());
}
