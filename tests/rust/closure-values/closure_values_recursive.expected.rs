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
impl<F: ?Sized> __SnClosure<F> {
    fn recursive(build: impl FnOnce(std::rc::Rc<std::cell::OnceCell<std::rc::Weak<F>>>) -> std::rc::Rc<F>) -> Self {
        let slot = std::rc::Rc::new(std::cell::OnceCell::new());
        let callable = build(slot.clone());
        let _ = slot.set(std::rc::Rc::downgrade(&callable));
        Self(callable)
    }
}
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
    let mut action: __SnClosure<dyn Fn(i64) -> i64> = { let () = (); self::__SnClosure::<dyn Fn(i64) -> i64>::recursive(move |action| std::rc::Rc::new(move |n: i64| -> i64 { if (n < 1) {
        return 1;
    }return ((self::__SnClosure(action.get().expect("recursive identity initialized").upgrade().expect("recursive callable alive"))).0)(__sn_checked_0((n).checked_sub(1), "Runtime error: integer overflow in subtraction"));})) }
;
    println!("{}", ((action.clone()).0)(3));
}
