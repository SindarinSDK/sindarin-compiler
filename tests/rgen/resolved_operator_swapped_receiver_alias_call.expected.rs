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
struct Item {
    value: i64,
}

impl Item {
    fn op_lt(&self, other: &mut Item) -> bool {
        ((other).value = __sn_checked_0(((other).value).checked_add(1), "Runtime error: integer overflow in addition"));
        return ((self).value < (other).value);
    }
}

fn increment(value: i64) -> i64 {
    return __sn_checked_0((value).checked_add(1), "Runtime error: integer overflow in addition");
}

fn makeFrom(item: Item, transform: __SnClosure<dyn Fn(i64) -> i64>) -> Item {
    return Item { value: ((transform.clone()).0)((item).value.clone()) };
}

fn main() {
    let mut item: Item = Item { value: 1 };
    println!("{}", (makeFrom(item, self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(increment)))).op_lt(&mut (item)));
    println!("{}", (item).value);
}
