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
fn invoke(callback: __SnClosure<dyn Fn(i64) -> String>, depth: i64) -> String {
    return ((callback.clone()).0)(depth.clone());
}

fn main() {
    let trace: std::rc::Rc<std::cell::RefCell<String>> = std::rc::Rc::new(std::cell::RefCell::new("r".to_string()));
    let calls: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(0));
    let mut action: __SnClosure<dyn Fn(i64) -> String> = { let (trace, calls, ) = (trace.clone(), calls.clone(), ); self::__SnClosure::<dyn Fn(i64) -> String>::recursive(move |action| std::rc::Rc::new(move |depth: i64| -> String { { let (__sn_string_part, __sn_cell) = (("a".to_string()).clone(), &trace); let mut __sn_string_place = __sn_cell.borrow_mut(); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };{ let __sn_cell = &calls; let __sn_previous = __sn_cell.get(); let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); __sn_cell.set(__sn_next); __sn_previous };if (depth > 0) {
        { let (__sn_string_part, __sn_cell) = ((invoke(self::__SnClosure(action.get().expect("recursive identity initialized").upgrade().expect("recursive callable alive")), __sn_checked_0((depth).checked_sub(1), "Runtime error: integer overflow in subtraction"))).clone(), &trace); let mut __sn_string_place = __sn_cell.borrow_mut(); __sn_string_place.push_str(&__sn_string_part); (*__sn_string_place).clone() };
    }return trace.borrow().clone();})) }
;
    println!("{}", ((action.clone()).0)(2));
    println!("{}", calls.get());
    println!("{}", ((action.clone()).0)(1));
    println!("{}", calls.get());
    println!("{}", trace.borrow().clone());
}
