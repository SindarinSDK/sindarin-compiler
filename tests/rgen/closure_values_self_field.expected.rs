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
#[derive(Clone, Debug, PartialEq)]
struct Holder {
    action: __SnClosure<dyn Fn(i64) -> i64>,
}

impl Holder {
    fn apply(&self, value: i64) -> i64 {
        return (((self).action.clone()).0)(value.clone());
    }
}

fn main() {
    let mut offset: i64 = 10;
    let mut holder: Holder = Holder { action: { let (offset, ) = (offset.clone(), ); self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { __sn_checked_0((value).checked_add(offset.clone()), "Runtime error: integer overflow in addition")})) }
 };
    let mut copied: Holder = holder.clone();
    ((holder).action = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |value: i64| -> i64 { __sn_checked_0((value).checked_add(100), "Runtime error: integer overflow in addition")})) }
);
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (holder).apply(1))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (copied).apply(2))); __sn_interpolated.push_str("\n"); __sn_interpolated });
}
