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
struct Pair {
    left: i64,
    right: i64,
}

fn main() {
    let mut pair: Pair = Pair { left: 1, right: 2 };
    let mut replace: __SnClosure<dyn Fn(i64) -> i64> = { let (pair, ) = (pair.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { let mut pair = pair.clone(); { let __sn_value = Pair { left: value, right: (pair.clone()).left }; pair = __sn_value.clone(); __sn_value };return __sn_checked_0((__sn_checked_0(((pair.clone()).left).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add((pair.clone()).right), "Runtime error: integer overflow in addition");})) }
;
    println!("{}", ((replace.clone()).0)(3));
    println!("{}", ((replace.clone()).0)(4));
    println!("{}", __sn_checked_0((__sn_checked_0(((pair).left).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add((pair).right), "Runtime error: integer overflow in addition"));
}
