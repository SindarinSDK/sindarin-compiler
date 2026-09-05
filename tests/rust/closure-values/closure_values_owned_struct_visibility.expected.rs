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
struct Point {
    x: i64,
    y: i64,
}

fn main() {
    let mut point: Point = Point { x: 10, y: 20 };
    let mut read: __SnClosure<dyn Fn() -> i64> = { let (point, ) = (point.clone(), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { __sn_checked_0(((point.clone()).x).checked_add((point.clone()).y), "Runtime error: integer overflow in addition")})) }
;
    (point = Point { x: 30, y: 40 });
    println!("{}", ((read.clone()).0)());
    println!("{}", __sn_checked_0(((point).x).checked_add((point).y), "Runtime error: integer overflow in addition"));
    if true {
        let mut point: Point = Point { x: 1, y: 2 };
        let mut shadowed: __SnClosure<dyn Fn() -> i64> = { let (point, ) = (point.clone(), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { __sn_checked_0(((point.clone()).x).checked_add((point.clone()).y), "Runtime error: integer overflow in addition")})) }
;
        (point = Point { x: 3, y: 4 });
        println!("{}", ((shadowed.clone()).0)());
        println!("{}", ((shadowed.clone()).0)());
    }
    println!("{}", ((read.clone()).0)());
}
