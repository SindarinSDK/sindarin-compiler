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
struct State {
    label: String,
    count: i64,
}

fn main() {
    let mut state: State = State { label: "seed".to_string(), count: 10 };
    let depth: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(0));
    let mut action: __SnClosure<dyn Fn(__SnClosure<dyn Fn() -> i64>) -> i64> = { let (state, ) = (state.clone(), ); self::__SnClosure::<dyn Fn(__SnClosure<dyn Fn() -> i64>) -> i64>(std::rc::Rc::new(move |callback: __SnClosure<dyn Fn() -> i64>| -> i64 { let mut state = state.clone(); let mut result: i64 = ((callback.clone()).0)();{ let __sn_value = __sn_checked_0((result).checked_add((state.clone()).count), "Runtime error: integer overflow in addition"); state.count = __sn_value.clone(); __sn_value };return (state.clone()).count;})) }
;
    let mut alias: __SnClosure<dyn Fn(__SnClosure<dyn Fn() -> i64>) -> i64> = action.clone();
    let mut callback: __SnClosure<dyn Fn() -> i64> = { let (depth, alias, ) = (depth.clone(), alias.clone(), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { { let (__sn_rhs, __sn_cell) = (1, &depth); let __sn_previous = __sn_cell.get(); let __sn_next = __sn_checked_0(__sn_previous.checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); __sn_cell.set(__sn_next); __sn_next };if (depth.get() == 1) {
        return ((alias.clone()).0)({ self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { 1})) }
 );
    }return 2;})) }
;
    println!("{}", ((action.clone()).0)(callback.clone().clone()));
    println!("{}", (state).count);
}
