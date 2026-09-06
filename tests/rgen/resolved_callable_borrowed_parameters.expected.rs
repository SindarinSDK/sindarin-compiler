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
#[derive(Clone, Copy, Debug, PartialEq)]
struct Dispatcher {
}

impl Dispatcher {
    fn reset(callback: &mut __SnClosure<dyn Fn(i64) -> i64>, marker: i64, order: &mut i64) {
        (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
        { *(callback) = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { __sn_checked_0((value).checked_add(10), "Runtime error: integer overflow in addition")})) }
; (*(callback)).clone() };
    }
}

fn marked(order: &mut i64, marker: i64) -> i64 {
    (*(order) = __sn_checked_0((__sn_checked_0((*(order)).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(marker), "Runtime error: integer overflow in addition"));
    return marker;
}

fn main() {
    let mut order: i64 = 0;
    let mut callback: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { __sn_checked_0((value).checked_add(1), "Runtime error: integer overflow in addition")})) }
;
    { let __sn_resolved_arg_0 = marked(&mut (order), 2); let __sn_resolved_arg_1 = &mut (order); Dispatcher::reset(&mut (callback), __sn_resolved_arg_0, __sn_resolved_arg_1) };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", order)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((callback.clone()).0)(2))); __sn_interpolated });
    { callback = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { __sn_checked_0((value).checked_add(100), "Runtime error: integer overflow in addition")})) }
; callback.clone() };
    println!("{}", ((callback.clone()).0)(1));
}
