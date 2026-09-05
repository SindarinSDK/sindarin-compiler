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
struct Counter {
    value: i64,
}

fn main() {
    let mut state: Counter = Counter { value: 10 };
    let mut outer: __SnClosure<dyn Fn() -> __SnClosure<dyn Fn(i64) -> i64>> = { let (state, ) = (state.clone(), ); self::__SnClosure::<dyn Fn() -> __SnClosure<dyn Fn(i64) -> i64>>(std::rc::Rc::new(move || -> __SnClosure<dyn Fn(i64) -> i64> { let mut state = state.clone(); { let __sn_rhs = 1; let __sn_place = &mut ((state).value); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };return { let (state, ) = (state.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |delta: i64| -> i64 { let mut state = state.clone(); { let __sn_rhs = delta; let __sn_place = &mut ((state).value); let __sn_next = __sn_checked_0((*__sn_place).checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_next };return (state.clone()).value;})) }
;})) }
;
    let mut first: __SnClosure<dyn Fn(i64) -> i64> = ((outer.clone()).0)();
    let mut second: __SnClosure<dyn Fn(i64) -> i64> = ((outer.clone()).0)();
    println!("{}", ((first.clone()).0)(2));
    println!("{}", ((first.clone()).0)(3));
    println!("{}", ((second.clone()).0)(4));
    println!("{}", (state).value);
}
